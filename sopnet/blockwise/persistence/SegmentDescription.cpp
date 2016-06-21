#include "SegmentDescription.h"
#include <imageprocessing/ConnectedComponent.h>
#include <sopnet/core/segments/Segments.h>


SegmentDescription::SegmentDescription(const Segment& segment) :
		_hashDirty(true),
		_cost(std::numeric_limits<double>::signaling_NaN()),
		_section(segment.getInterSectionInterval()),
		_boundingBox(0, 0, 0, 0) {

	// get the 2D bounding box of the segment
	for (boost::shared_ptr<Slice> slice : segment.getSlices()) {

		if (_boundingBox.area() == 0)
			_boundingBox = slice->getComponent()->getBoundingBox();
		else
			_boundingBox.fit(slice->getComponent()->getBoundingBox());
	}

	// add slice hashes
	if (segment.getDirection() == Left) {

		for (boost::shared_ptr<Slice> slice : segment.getTargetSlices())
			addLeftSlice(slice->hashValue());
		for (boost::shared_ptr<Slice> slice : segment.getSourceSlices())
			addRightSlice(slice->hashValue());

	} else {

		for (boost::shared_ptr<Slice> slice : segment.getTargetSlices())
			addRightSlice(slice->hashValue());
		for (boost::shared_ptr<Slice> slice : segment.getSourceSlices())
			addLeftSlice(slice->hashValue());
	}
}

SegmentHash
SegmentDescription::getHash() const {

	if (_hashDirty) {
		_hash = hash_value(_leftSliceHashes, _rightSliceHashes);

		_hashDirty = false;
	}

	return _hash;
}

SegmentType
SegmentDescription::getType() const {

	std::size_t leftSize = _leftSliceHashes.size();
	std::size_t rightSize = _rightSliceHashes.size();

	if (leftSize == 0 || rightSize == 0) return EndSegmentType;
	else if (leftSize == 1 && rightSize == 1) return ContinuationSegmentType;
	else return BranchSegmentType;
}

Direction
SegmentDescription::getDirection() const {

	std::size_t leftSize = _leftSliceHashes.size();
	std::size_t rightSize = _rightSliceHashes.size();

	switch (getType()) {

		case EndSegmentType:
			return (leftSize > rightSize) ? Right : Left;
			break;
		case BranchSegmentType:
			return (leftSize > rightSize) ? Left : Right;
			break;
		default:
			return Left;
			break;
	}
}

boost::shared_ptr<Segment>
SegmentDescription::asSegment(const std::map<SliceHash, boost::shared_ptr<Slice> >& sliceHashMap) const {

	boost::shared_ptr<Segment> segment;

	switch (getType()) {

		case EndSegmentType: {
			SliceHash sliceHash = getDirection() == Left ?
					getRightSlices()[0] :
					getLeftSlices()[0];
			segment = boost::make_shared<EndSegment>(
					Segment::getNextSegmentId(),
					getDirection(),
					sliceHashMap.at(sliceHash));
			break;
		}

		case ContinuationSegmentType:
			segment = boost::make_shared<ContinuationSegment>(
					Segment::getNextSegmentId(),
					getDirection(),
					sliceHashMap.at(getRightSlices()[0]),
					sliceHashMap.at(getLeftSlices()[0]));
			break;

		case BranchSegmentType:
			if (getDirection() == Left) {
				segment = boost::make_shared<BranchSegment>(
						Segment::getNextSegmentId(),
						Left,
						sliceHashMap.at(getRightSlices()[0]),
						sliceHashMap.at(getLeftSlices()[0]),
						sliceHashMap.at(getLeftSlices()[1]));
			} else {
				segment = boost::make_shared<BranchSegment>(
						Segment::getNextSegmentId(),
						Right,
						sliceHashMap.at(getLeftSlices()[0]),
						sliceHashMap.at(getRightSlices()[0]),
						sliceHashMap.at(getRightSlices()[1]));
			}
			break;

		default:
			break; // Should not occur.
	}

	return segment;
}
