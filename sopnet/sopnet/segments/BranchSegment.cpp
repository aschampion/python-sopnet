#include <imageprocessing/ConnectedComponent.h>
#include "BranchSegment.h"

BranchSegment::BranchSegment(
		unsigned int id,
		Direction direction,
		boost::shared_ptr<Slice> sourceSlice,
		boost::shared_ptr<Slice> targetSlice1,
		boost::shared_ptr<Slice> targetSlice2) :
	Segment(
			id,
			direction,
			(sourceSlice->getComponent()->getCenter()*sourceSlice->getComponent()->getSize()   +
			 targetSlice1->getComponent()->getCenter()*targetSlice1->getComponent()->getSize() +
			 targetSlice2->getComponent()->getCenter()*targetSlice2->getComponent()->getSize())/
			 (sourceSlice->getComponent()->getSize() + targetSlice1->getComponent()->getSize() + targetSlice2->getComponent()->getSize()),
			sourceSlice->getSection() + (direction == Left ? 0 : 1)),
	_sourceSlice(sourceSlice),
	_targetSlice1(targetSlice1),
	_targetSlice2(targetSlice2) { setHash(); }

boost::shared_ptr<Slice>
BranchSegment::getSourceSlice() const {

	return _sourceSlice;
}

boost::shared_ptr<Slice>
BranchSegment::getTargetSlice1() const {

	return _targetSlice1;
}

boost::shared_ptr<Slice>
BranchSegment::getTargetSlice2() const {

	return _targetSlice2;
}

std::vector<boost::shared_ptr<Slice> >
BranchSegment::getSlices() const {

	std::vector<boost::shared_ptr<Slice> > slices;

	slices.push_back(getSourceSlice());
	slices.push_back(getTargetSlice1());
	slices.push_back(getTargetSlice2());

	return slices;
}

SegmentType
BranchSegment::getType() const
{
    return BranchSegmentType;
}

