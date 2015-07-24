#include <boost/function.hpp>

#include <imageprocessing/ConnectedComponent.h>
#include <util/ProgramOptions.h>
#include "EndSegment.h"
#include "ContinuationSegment.h"
#include "BranchSegment.h"
#include "BranchSegment.h"
#include "SegmentExtractor.h"

static logger::LogChannel segmentextractorlog("segmentextractorlog", "[SegmentExtractor] ");

util::ProgramOption optionContinuationOverlapThreshold(
		util::_module           = "sopnet.segments",
		util::_long_name        = "continuationOverlapThreshold",
		util::_description_text = "The minimal normalized overlap between slices to consider them for continuation segment hypotheses.",
		util::_default_value    = 0.5);

util::ProgramOption optionMinContinuationPartners(
		util::_module           = "sopnet.segments",
		util::_long_name        = "minContinuationPartners",
		util::_description_text = "The minimal number of continuation partners for each slice, even if they do not meet the overlap threshold.",
		util::_default_value    = 0);

util::ProgramOption optionBranchOverlapThreshold(
		util::_module           = "sopnet.segments",
		util::_long_name        = "branchOverlapThreshold",
		util::_description_text = "The minimal normalized overlap between slices to consider them for branch segment hypotheses.",
		util::_default_value    = 0.5);

util::ProgramOption optionBranchSizeRatioThreshold(
		util::_module           = "sopnet.segments",
		util::_long_name        = "branchSizeRatioThreshold",
		util::_description_text = "The minimal size ratio (between 0 and 1) of the two target slices of a branch. The ratio is the size of the smaller region divided by the bigger region, i.e., 1 if both regions are of the same size, converging towards 0 for differently sized regions.",
		util::_default_value    = 0.5);

util::ProgramOption optionSliceDistanceThreshold(
		util::_module           = "sopnet.segments",
		util::_long_name        = "sliceDistanceThreshold",
		util::_description_text = "The maximal slice distance between slices to consider them for segment hypotheses."
		                          "The slice distance is the average minimal distance of a pixel from one slice to any "
		                          "pixel of another slice.",
		util::_default_value    = 10);

util::ProgramOption optionDisableBranches(
		util::_module           = "sopnet.segments",
		util::_long_name        = "disableBranches",
		util::_description_text = "Disable the extraction of branch segments.");


SegmentExtractor::SegmentExtractor() :
	_segments(new Segments()),
	_linearConstraints(new LinearConstraints()),
	_overlap(false /* don't normalize */, false /* don't align */),
	_continuationOverlapThreshold(optionContinuationOverlapThreshold.as<double>()),
	_branchOverlapThreshold(optionBranchOverlapThreshold.as<double>()),
	_minContinuationPartners(optionMinContinuationPartners.as<unsigned int>()),
	_branchSizeRatioThreshold(optionBranchSizeRatioThreshold.as<double>()),
	_sliceDistanceThreshold(optionSliceDistanceThreshold.as<double>()),
	_slicesChanged(true),
	_conflictSetsChanged(true) {

	registerInput(_prevSlices, "previous slices");
	registerInput(_nextSlices, "next slices");
	registerInput(_prevConflictSets, "previous conflict sets", pipeline::Optional);
	registerInput(_nextConflictSets, "next conflict sets", pipeline::Optional);
	registerInput(_forceExplanation, "force explanation", pipeline::Optional);

	registerOutput(_segments, "segments");
	registerOutput(_linearConstraints, "linear constraints");

	_prevSlices.registerCallback(&SegmentExtractor::onSlicesModified, this);
	_nextSlices.registerCallback(&SegmentExtractor::onSlicesModified, this);
	_prevConflictSets.registerCallback(&SegmentExtractor::onConflictSetsModified, this);
	_nextConflictSets.registerCallback(&SegmentExtractor::onConflictSetsModified, this);
}

void
SegmentExtractor::onSlicesModified(const pipeline::Modified&) {

	_slicesChanged = true;
}

void
SegmentExtractor::onConflictSetsModified(const pipeline::Modified&) {

	_conflictSetsChanged = true;
}

void
SegmentExtractor::updateOutputs() {
	
	
	if (_slicesChanged) {
		extractSegments();
		_slicesChanged = false;
	}

	if (_conflictSetsChanged && _prevConflictSets.isSet()) {

		assembleLinearConstraints();
		_conflictSetsChanged = false;
	}

	_distance.clearCache();
	
	//LOG_DEBUG(segmentextractorlog) << "
}

void
SegmentExtractor::extractSegments() {

	LOG_DEBUG(segmentextractorlog)
			<< "previous sections contains " << _prevSlices->size() << " slices,"
			<< "next sections contains "     << _nextSlices->size() << " slices" << std::endl;

	LOG_ALL(segmentextractorlog) << "Branch overlap threshold: " << _branchOverlapThreshold << std::endl;
	LOG_ALL(segmentextractorlog) << "Branch size ratio threshold: " << _branchSizeRatioThreshold << std::endl;

			
	buildOverlapMap();

	unsigned int oldSize = 0;

	LOG_DEBUG(segmentextractorlog) << "extracting segments..." << std::endl;

	LOG_DEBUG(segmentextractorlog) << "extracting continuations to next section..." << std::endl;

	// for all slices in previous section...
	for (boost::shared_ptr<Slice> prev : *_prevSlices) {

		LOG_ALL(segmentextractorlog) << "found " << _nextOverlaps[prev].size() << " partners" << std::endl;

		// ...and all overlapping slices in the next section...
		for (const auto& pair : _nextOverlaps[prev]) {

			// ...try to extract the segment
			extractSegment(prev, pair.second, pair.first);
		}
	}

	LOG_DEBUG(segmentextractorlog) << _segments->size() << " segments extraced so far (+" << (_segments->size() - oldSize) << ")" << std::endl;
	oldSize = _segments->size();

	LOG_DEBUG(segmentextractorlog) << "ensuring at least " << _minContinuationPartners << " continuation partners for each slice..." << std::endl;

	ensureMinContinuationPartners();

	LOG_DEBUG(segmentextractorlog) << _segments->size() << " segments extraced so far (+" << (_segments->size() - oldSize) << ")" << std::endl;
	oldSize = _segments->size();

	if (!optionDisableBranches) {

		LOG_DEBUG(segmentextractorlog) << "extracting bisections from previous to next section..." << std::endl;

		for (boost::shared_ptr<Slice> prev : *_prevSlices) {

			for (const auto& pair1 : _nextOverlaps[prev]) {
				for (const auto& pair2 : _nextOverlaps[prev]) {

					if (pair1.second->getId() <= pair2.second->getId())
						continue;

					if (!_nextSlices->areConflicting(pair1.second->getId(), pair2.second->getId()))
						extractSegment(prev, pair1.second, pair2.second, Right, pair1.first, pair2.first);
				}
			}
		}

		LOG_DEBUG(segmentextractorlog) << "extracting bisections from next to previous section..." << std::endl;

		for (boost::shared_ptr<Slice> next : *_nextSlices) {

			for (const auto& pair1 : _prevOverlaps[next]) {
				for (const auto& pair2 : _prevOverlaps[next]) {

					if (pair1.second->getId() <= pair2.second->getId())
						continue;

					if (!_prevSlices->areConflicting(pair1.second->getId(), pair2.second->getId()))
						extractSegment(next, pair1.second, pair2.second, Left, pair1.first, pair2.first);
				}
			}
		}

		LOG_DEBUG(segmentextractorlog) << _segments->size() << " segments extraced so far (+" << (_segments->size() - oldSize) << ")" << std::endl;
	}

	LOG_DEBUG(segmentextractorlog) << "extracting ends from previous section..." << std::endl;

	// end segments for every previous slice
	for (boost::shared_ptr<Slice> prevSlice : *_prevSlices)
		extractSegment(prevSlice, Right);

	LOG_DEBUG(segmentextractorlog) << "extracting ends to next section..." << std::endl;

	// end segments for every next slice
	for (boost::shared_ptr<Slice> nextSlice : *_nextSlices)
		extractSegment(nextSlice, Left);

	LOG_DEBUG(segmentextractorlog) << _segments->size() << " segments extraced so far (+" << (_segments->size() - oldSize) << ")" << std::endl;
	oldSize = _segments->size();

	LOG_DEBUG(segmentextractorlog) << "extracted " << _segments->size() << " segments in total" << std::endl;
	LOG_DEBUG(segmentextractorlog) << "by type: " << _segments->getEnds().size() << " ends, " << 
		_segments->getContinuations().size() << " continuations, and " << 
		_segments->getBranches().size() << " branches." << std::endl;
}

void
SegmentExtractor::ensureMinContinuationPartners() {

	// for all slices with fewer than the required number of partners...
	for (boost::shared_ptr<Slice> prev : *_prevSlices) {

		unsigned int prevId = prev->getId();

		unsigned int numPartners = _continuationPartners[prevId].size();

		if (numPartners < _minContinuationPartners) {

			// sort overlapping slices by overlap
			std::sort(_nextOverlaps[prev].rbegin(), _nextOverlaps[prev].rend());

			// ...and all overlapping slices in the next section...
			for (const auto& pair : _nextOverlaps[prev]) {

				unsigned int nextId = pair.second->getId();

				// ...if not already a partner...
				if (std::count(_continuationPartners[prevId].begin(), _continuationPartners[prevId].end(), nextId))
					continue;

				// ...extract the segment
				extractSegment(prev, pair.second);

				numPartners++;

				if (numPartners == _minContinuationPartners)
					break;
			}
		}
	}

	// for all slices with fewer than the required number of partners
	for (boost::shared_ptr<Slice> next : *_nextSlices) {

		unsigned int nextId = next->getId();

		unsigned int numPartners = _continuationPartners[nextId].size();

		if (numPartners < _minContinuationPartners) {

			// ...and all overlapping slices in the prev section...
			for (const auto& pair : _prevOverlaps[next]) {

				unsigned int prevId = pair.second->getId();

				// ...if not already a partner...
				if (std::count(_continuationPartners[nextId].begin(), _continuationPartners[nextId].end(), prevId))
					continue;

				// ...extract the segment
				extractSegment(pair.second, next);

				numPartners++;

				if (numPartners == _minContinuationPartners)
					break;
			}
		}
	}
}

void
SegmentExtractor::buildOverlapMap() {

	LOG_DEBUG(segmentextractorlog) << "building overlap maps..." << std::endl;

	_prevOverlaps.clear();
	_nextOverlaps.clear();

	unsigned int i = 0;
	for (boost::shared_ptr<Slice> prev : *_prevSlices) {
		for (boost::shared_ptr<Slice> next : *_nextSlices) {

			double value;

			if (_overlap.exceeds(*prev, *next, 0, value)) {

				_nextOverlaps[prev].push_back(std::make_pair(static_cast<unsigned int>(value), next));
				_prevOverlaps[next].push_back(std::make_pair(static_cast<unsigned int>(value), prev));
			}
		}

		if (i % (std::max(static_cast<unsigned int>(1), _prevSlices->size()/10)) == 0) {

			LOG_DEBUG(segmentextractorlog) << round(static_cast<double>(i)*100/std::max(static_cast<unsigned int>(1), _prevSlices->size())) << "%" << std::endl;
		}

		i++;
	}

	LOG_DEBUG(segmentextractorlog) << "done." << std::endl;
}

bool
SegmentExtractor::extractSegment(boost::shared_ptr<Slice> slice, Direction direction) {

	boost::shared_ptr<EndSegment> segment = boost::make_shared<EndSegment>(Segment::getNextSegmentId(), direction, slice);

	_segments->add(segment);
	
	LOG_ALL(segmentextractorlog) << "Created segment " << segment->getId() << " from slice " <<
		slice->getId() << std::endl;

	// only for ends that have the slice on the left side
	if (direction == Right)
		_sliceSegments[slice->getId()].push_back(segment->getId());

	return true;
}

bool
SegmentExtractor::extractSegment(boost::shared_ptr<Slice> prevSlice, boost::shared_ptr<Slice> nextSlice, unsigned int overlap) {

	double normalizedOverlap = Overlap::normalize(*prevSlice, *nextSlice, overlap);

	if (normalizedOverlap < _continuationOverlapThreshold) {

		LOG_ALL(segmentextractorlog) << "discarding this segment hypothesis" << std::endl;
		return false;
	}

	LOG_ALL(segmentextractorlog) << "accepting this segment hypothesis" << std::endl;

	extractSegment(prevSlice, nextSlice);

	return true;
}


void
SegmentExtractor::extractSegment(boost::shared_ptr<Slice> prevSlice, boost::shared_ptr<Slice> nextSlice) {

	boost::shared_ptr<ContinuationSegment> segment = boost::make_shared<ContinuationSegment>(Segment::getNextSegmentId(), Right, prevSlice, nextSlice);

	_segments->add(segment);

	// only for the left slice
	_sliceSegments[prevSlice->getId()].push_back(segment->getId());

	_continuationPartners[prevSlice->getId()].push_back(nextSlice->getId());
	_continuationPartners[nextSlice->getId()].push_back(prevSlice->getId());
	
	LOG_ALL(segmentextractorlog) << "Created segment " << segment->getId() << " from slices " <<
		prevSlice->getId() << " and " << nextSlice->getId() << std::endl;
}

bool
SegmentExtractor::extractSegment(
		boost::shared_ptr<Slice> source,
		boost::shared_ptr<Slice> target1,
		boost::shared_ptr<Slice> target2,
		Direction direction,
		unsigned int overlap1,
		unsigned int overlap2) {

	double normalizedOverlap = Overlap::normalize(*target1, *target2, *source, overlap1 + overlap2);

	LOG_ALL(segmentextractorlog) << "Branch normalized overlap: " << normalizedOverlap << std::endl;
	
	if (normalizedOverlap > 1) {

		LOG_ALL(segmentextractorlog) << normalizedOverlap << std::endl;
		LOG_ALL(segmentextractorlog) << overlap1 << std::endl;
		LOG_ALL(segmentextractorlog) << overlap2 << std::endl;
		LOG_ALL(segmentextractorlog) << target1->getComponent()->getSize() << std::endl;
		LOG_ALL(segmentextractorlog) << target2->getComponent()->getSize() << std::endl;
		LOG_ALL(segmentextractorlog) << source->getComponent()->getSize() << std::endl;
		LOG_ALL(segmentextractorlog) << std::endl;
	}

	if (normalizedOverlap < _branchOverlapThreshold)
		return false;

	unsigned int size1 = target1->getComponent()->getSize();
	unsigned int size2 = target2->getComponent()->getSize();

	double sizeRatio = static_cast<double>(std::min(size1, size2))/std::max(size1, size2);

	LOG_ALL(segmentextractorlog) << "Branch size ratio: " << sizeRatio << std::endl;
	
	if (sizeRatio < _branchSizeRatioThreshold)
		return false;

	//double avgSliceDistance, maxSliceDistance;

	//_distance(*target1, *target2, *source, true, false, avgSliceDistance, maxSliceDistance);

	//if (maxSliceDistance >= _sliceDistanceThreshold)
		//return;

	boost::shared_ptr<BranchSegment> segment = boost::make_shared<BranchSegment>(Segment::getNextSegmentId(), direction, source, target1, target2);

	_segments->add(segment);

	// only for the left slice(s)

	if (direction == Left) {

		_sliceSegments[target1->getId()].push_back(segment->getId());
		_sliceSegments[target2->getId()].push_back(segment->getId());

	} else {

		_sliceSegments[source->getId()].push_back(segment->getId());
	}

	LOG_ALL(segmentextractorlog) << "Created segment " << segment->getId() << " from slices " <<
		source->getId() << ", " << target1->getId() << ", and " << target2->getId() << std::endl;
	
	return true;
}

void
SegmentExtractor::assembleLinearConstraints() {

	LOG_DEBUG(segmentextractorlog) << "assembling linear constraints..." << std::endl;

	_linearConstraints->clear();

	/* For each conflict set on the slices, create a corresponding linear
	 * constraint on the segments by replacing every sliceId by all segmentIds
	 * that are using this sliceId on the left side.
	 */
	for (const ConflictSet& conflictSet : *_prevConflictSets)
		assembleLinearConstraint(conflictSet);

	/* If linear constraints were also given for the next slice, consider them
	 * as well.
	 */
	if (_nextConflictSets.isSet()) {

		LOG_DEBUG(segmentextractorlog) << "using conflict sets of next slice" << std::endl;

		for (const ConflictSet& conflictSet : *_nextConflictSets)
			assembleLinearConstraint(conflictSet);
	}

	LOG_DEBUG(segmentextractorlog) << "assembled " << _linearConstraints->size() << " linear constraints" << std::endl;
}

void
SegmentExtractor::assembleLinearConstraint(const ConflictSet& conflictSet) {

	LinearConstraint constraint;

	// for each slice in the constraint
	for (unsigned int sliceId : conflictSet.getSlices()) {

		// for all the segments that involve this slice
		const std::vector<unsigned int> segmentIds = _sliceSegments[sliceId];

		for (unsigned int segmentId : segmentIds)
			constraint.setCoefficient(segmentId, 1.0);
	}

	if (*_forceExplanation)
		constraint.setRelation(Equal);
	else
		constraint.setRelation(LessEqual);

	constraint.setValue(1);

	LOG_ALL(segmentextractorlog) << "created constraint " << constraint << std::endl;

	_linearConstraints->add(constraint);
}