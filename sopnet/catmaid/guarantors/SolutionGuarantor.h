#ifndef SOLUTION_GUARANTOR_H__
#define SOLUTION_GUARANTOR_H__

#include <boost/shared_ptr.hpp>

#include <catmaid/persistence/SegmentStore.h>
#include <catmaid/persistence/SliceStore.h>
#include <catmaid/blocks/Core.h>

#include <sopnet/segments/SegmentHash.h>
#include <sopnet/inference/LinearConstraints.h>
#include <sopnet/inference/LinearObjective.h>

class SolutionGuarantor {

public:

	/**
	 * Create a new SolutionGuarantor using the given database stores.
	 *
	 * @param segmentStore
	 *              The SegmentStore to use to retrieve segments and store the  
	 *              solution.
	 *
	 * @param sliceStore
	 *              The slice store to retrieve conflict sets on slices.
	 *
	 * @param corePadding
	 *              The number of blocks to pad around a core in order to 
	 *              eliminate border effects. The solution will be computed on 
	 *              the padded core, but only the solution of the core will be 
	 *              stored.
	 *
	 * @param featureWeights
	 *              Linear coefficients to compute the costs from the segment 
	 *              features.
	 */
	SolutionGuarantor(
			boost::shared_ptr<SegmentStore> segmentStore,
			boost::shared_ptr<SliceStore>   sliceStore,
			unsigned int                    corePadding,
			const std::vector<double>&      featureWeights);

	/**
	 * Get the solution for the given cores.
	 *
	 * @param core
	 *              The core to compute the solution for.
	 */
	Blocks guaranteeSolution(const Core& core);

private:

	// get all blocks of the padded core
	Blocks getPaddedCoreBlocks(const Core& core);

	std::vector<SegmentHash> computeSolution(
			const SegmentDescriptions& segments,
			const ConflictSets& conflictSets);

	boost::shared_ptr<LinearConstraints> createConstraints(
			const SegmentDescriptions& segments,
			const ConflictSets&        conflictSets);

	boost::shared_ptr<LinearObjective> createObjective(const SegmentDescriptions& segments);

	void addOverlapConstraints(
			const SegmentDescriptions& segments,
			const ConflictSets&        conflictSets,
			LinearConstraints&         constraints);

	void addContinuationConstraints(
			const SegmentDescriptions& segments,
			LinearConstraints&         constraints);

	double getCost(const std::vector<double>& features);

	boost::shared_ptr<SegmentStore> _segmentStore;
	boost::shared_ptr<SliceStore>   _sliceStore;

	unsigned int _corePadding;

	// mappings from segment hashes to their variable number in the ILP
	std::map<SegmentHash, unsigned int> _hashToVariable;
	std::map<unsigned int, SegmentHash> _variableToHash;

	// mappings from slice hashes to hashes of segments that use the slice 
	// either on the left or right side
	std::map<SliceHash, std::vector<SegmentHash> > _leftSliceToSegments;
	std::map<SliceHash, std::vector<SegmentHash> > _rightSliceToSegments;

	// the feature weights
	std::vector<double> _weights;
};

#endif //SOLUTION_GUARANTOR_H__
