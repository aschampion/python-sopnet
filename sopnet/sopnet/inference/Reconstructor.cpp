#include <util/Logger.h>
#include <util/foreach.h>
#include "Reconstructor.h"

static logger::LogChannel reconstructorlog("reconstructorlog", "[Reconstructor] ");

Reconstructor::Reconstructor() :
	_reconstruction(new Segments()),
	_discardedSegments(new Segments()) {

	registerInput(_solution, "solution");
	registerInput(_segments, "segments");
	registerOutput(_reconstruction, "reconstruction");
	registerOutput(_discardedSegments, "discarded segments");
}

void
Reconstructor::updateOutputs() {

	LOG_DEBUG(reconstructorlog) << "reconstructing segments from solution" << std::endl;

	updateReconstruction();
}

void
Reconstructor::updateReconstruction() {

	// remove all previous segment in the reconstruction
	_reconstruction->clear();
	_discardedSegments->clear();

	LOG_DEBUG(reconstructorlog) << "Got " << _segments->size() << " segments in total" << std::endl;
	LOG_DEBUG(reconstructorlog) << _segments->getEnds().size() << " ends, " <<
		_segments->getContinuations().size() << " continuations, and " << 
		_segments->getBranches().size() << " branches." << std::endl;
	LOG_DEBUG(reconstructorlog) << "Solution contains " << _solution->size() << " things" << std::endl;
	
	LOG_ALL(reconstructorlog) << "Solution consists of segments: ";

	_currentSegmentNum = 0;

	foreach (boost::shared_ptr<EndSegment> segment, _segments->getEnds())
		probe(segment);

	foreach (boost::shared_ptr<ContinuationSegment> segment, _segments->getContinuations())
		probe(segment);

	foreach (boost::shared_ptr<BranchSegment> segment, _segments->getBranches())
		probe(segment);

	LOG_ALL(reconstructorlog) << std::endl;
	
	LOG_DEBUG(reconstructorlog) << "Reconstruction contains " << _reconstruction->size() << std::endl;
}

template <typename SegmentType>
void
Reconstructor::probe(boost::shared_ptr<SegmentType> segment) {

	if (_currentSegmentNum < _solution->size() && (*_solution)[_currentSegmentNum] == 1.0) {

		_reconstruction->add(segment);

		LOG_ALL(reconstructorlog) << segment->getId() << " ";

	} else  {

		_discardedSegments->add(segment);
	}

	_currentSegmentNum++;
}
