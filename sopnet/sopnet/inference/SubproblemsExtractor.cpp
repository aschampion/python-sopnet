#include <util/ProgramOptions.h>
#include <util/Logger.h>
#include "SubproblemsExtractor.h"

util::ProgramOption optionSubproblemsSize(
		util::_module           = "sopnet.inference",
		util::_long_name        = "subproblemsSize",
		util::_description_text = "The size of the subproblems in sections.");

util::ProgramOption optionSubproblemsOverlap(
		util::_module           = "sopnet.inference",
		util::_long_name        = "subproblemsOverlap",
		util::_description_text = "The overlap between neighboring subproblems in sections.");

logger::LogChannel subproblemsextractorlog("subproblemsextractorlog", "[SubproblemsExtractor] ");

SubproblemsExtractor::SubproblemsExtractor() :
	_subproblems(new Subproblems()) {

	registerInput(_objective, "objective");
	registerInput(_constraints, "linear constraints");
	registerInput(_configuration, "problem configuration");

	registerOutput(_subproblems, "subproblems");
}

void
SubproblemsExtractor::updateOutputs() {

	// The big picture: Create a working problem from a set of segments, dump it 
	// as subproblems, solve it with SCALAR, read solution, map it back to 
	// segments.
	//
	// Later, the working problem is just a part of the global problem and is 
	// decomposed into only a few subproblems for consolidation. SCALAR tries to 
	// solve them. SCALAR subproblem variable ids are volatile and are only 
	// known to SCALAR.  Thus, we cannot have a warm start if the same 
	// subproblem is involved another time (which will be very likely) -- at 
	// least not trivially.  Hence, it would be nice if the subproblem state 
	// could be stored independent of the SCALAR subproblem variables, in a map 
	// with the subproblem hash (from all involved global segment ids) as key.  
	// If the SCALAR subproblem is created the same way to yield the same ILP 
	// every time, then we can re-use the previous solver state.  That sounds 
	// feasible.
	//
	// More specific:
	//
	//   Global problem - this is the problem for the whole stack. Doesn't fit 
	//   into the memory, we never see it at once. Variables are implicitly 
	//   represented as segments, which have a globally unique id.
	//
	//   Partial problem - subset of the global problem. Grows as more and more 
	//   of the global problem is explored. Eventually, will not fit into the 
	//   memory. No need to see it at once. Variables are implicitly represented 
	//   as segments, which have a globally unique id.
	//
	//   Working problem - this is a part of the huge problem of variable size.  
	//   Fits into the memory, but might be too big to be solved at once. Runs 
	//   like a sliding window over the global problem to grow the partial 
	//   problem. Variables are contigous sequences in [0,...,n-1] (n number of 
	//   variables in working problem).  Constraints and mapping to segment ids 
	//   are converted/created on demand.
	//
	//   Subproblem - a part of a working problem of constant size. One 
	//   subproblem is assumed to be solvable at once. Variables are subsets of 
	//   [0,...,n] (n number of variables in working problem).
	//
	// What happens:
	//
	//   User selects working problem. Working problem is decomposed into 
	//   subproblems.  Subproblems are solved with SCALAR, result is presented 
	//   as solution to working problem. Solution is stored as solution to 
	//   partial problem.
	//
	//   User augments partial problem. New part and overlapping previous 
	//   subproblems are new working problem.  New subproblems are created.  
	//   Previous subproblems are re-created (and their state is restored for 
	//   the ILP solver). Solved with SCALAR, result augments global solution. 
	//   Ideally, it is not necessary to re-create and re-evaluate more previous 
	//   subproblems.


	// According to our nomenclature, what we get here as _objective, 
	// _constraints, and _configuration is a working problem.
	//
	// We decompose the working problem into subproblems. Variable ids are kept, 
	// thus they are non-contigous for the subproblems. Data structure 
	// LinearObjective is not made for that, but we don't need it if we dump the 
	// problem on-the-fly.
	//
	// Basically, we annotate each constraint with subproblem numbers and create 
	// the SCALAR dump from that. What we need: all involved variables (these 
	// are all variables in the working problem), subproblem numbers for each 
	// contraint and each unary term (of which we have one per variable).

	// compute the sizes of the subproblems

	unsigned int subproblemsSize    = optionSubproblemsSize;
	unsigned int subproblemsOverlap = optionSubproblemsOverlap;
	unsigned int minInterSectionInterval = _configuration->getMinInterSectionInterval();
	unsigned int maxInterSectionInterval = _configuration->getMaxInterSectionInterval();

	// collect the working problem in a single problem
	boost::shared_ptr<Problem> problem = boost::make_shared<Problem>();
	problem->setObjective(_objective);
	problem->setLinearConstraints(_constraints);
	problem->setConfiguration(_configuration);

	// create the subproblem data structure
	_subproblems->clear();
	_subproblems->setProblem(problem);

	LOG_DEBUG(subproblemsextractorlog)
			<< "decomposing problem with extents " << minInterSectionInterval
			<< "-" << maxInterSectionInterval << " into pieces of "
			<< subproblemsSize << " with overlap of "
			<< subproblemsOverlap << std::endl;

	// 1D decomposition of the working problem
	unsigned int subproblemId = 0;
	for (unsigned int startSubproblem = minInterSectionInterval; startSubproblem < maxInterSectionInterval; startSubproblem += subproblemsSize - subproblemsOverlap) {

		// the first inter-section interval that is not part of the subproblem
		unsigned int endSubproblem = startSubproblem + subproblemsSize;

		LOG_DEBUG(subproblemsextractorlog) << "creating subproblem " << subproblemId << " for inter-section intervals " << startSubproblem << "-" << (endSubproblem-1) << std::endl;

		// get all working problem variable ids for this subproblem
		std::vector<unsigned int> workingVarIds = _configuration->getVariables(startSubproblem, endSubproblem);

		LOG_DEBUG(subproblemsextractorlog) << "this subproblem contains " << workingVarIds.size() << " variables" << std::endl;

		// remember mapping of subproblem variable ids to this subproblem 
		// (needed for unary terms)
		foreach (unsigned int workingVarId, workingVarIds) {

			LOG_ALL(subproblemsextractorlog) << "assigning variable " << workingVarId << " to subproblem " << subproblemId << std::endl;
			_subproblems->assignVariable(workingVarId, subproblemId);
		}

		// find all working problem constraints that involve the subproblem 
		// variable ids
		std::vector<unsigned int> constraints = _constraints->getConstraints(workingVarIds);

		// remember mapping of constraints to this subproblem
		foreach (unsigned int i, constraints) {

			LinearConstraint& constraint = (*_constraints)[i];

			// There are two types of constraints: [expr]≤1 and [expr]=0.  The 
			// first is defined within one inter-section interval and ensures 
			// that at most one of conflicting segments is picked.  The second 
			// is defined between two inter-section intervals and
			// ensures continuation.
			//
			// Always accept the first type. Accept the second type only if 
			// it is fully contained in our problems variables. To simplify 
			// things (and be more general), accept constraints only if they
			// are fully contained in our variables.

			// Working variable ids have already been assigned to subproblem 
			// ids. We can thus just ask for that.
			unsigned int workingVarId;
			double _;
			bool addConstraint = true;
			foreach (boost::tie(workingVarId, _), constraint.getCoefficients()) {

				// get all subproblems that are assigned to the working variable
				std::set<unsigned int>& assignedSubproblems = _subproblems->getVariableSubproblems(workingVarId);

				// does it containt the current subproblem?
				if (!assignedSubproblems.count(subproblemId)) {

					addConstraint = false;
					break;
				}
			}

			if (addConstraint) {

				LOG_ALL(subproblemsextractorlog) << "assigning constraint " << i << " to subproblem " << subproblemId << std::endl;
				_subproblems->assignConstraint(i, subproblemId);
			}
		}

		subproblemId++;
	}
}
