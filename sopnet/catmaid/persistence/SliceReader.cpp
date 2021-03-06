#include "SliceReader.h"
#include <sopnet/block/Block.h>


#include <util/Logger.h>
logger::LogChannel slicereaderlog("slicereaderlog", "[SliceReader] ");

SliceReader::SliceReader()
{
	registerInput(_blocks, "blocks");
	registerInput(_store, "store");
	
	registerOutput(_slices, "slices");
	registerOutput(_conflictSets, "conflict sets");
}

void SliceReader::updateOutputs()
{
	_slices = new Slices();
	_conflictSets = new ConflictSets();

	if (_blocks->length() == 0)
		return;

	pipeline::Value<Slices> slices;
	pipeline::Value<ConflictSets> conflict;

	LOG_DEBUG(slicereaderlog) << "Retrieving block slices" << std::endl;

	slices = _store->retrieveSlices(_blocks);
	conflict = _store->retrieveConflictSets(slices);

	LOG_DEBUG(slicereaderlog) << "Retrieved " << slices->size() << " slices." << std::endl;
	
	std::sort(slices->begin(), slices->end(), SliceReader::slicePtrComparator);
	
	*_slices = *slices;
	*_conflictSets = *conflict;

	LOG_DEBUG(slicereaderlog) << "Return " << _slices->size() << " slices." << std::endl;
}

bool
SliceReader::slicePtrComparator(const boost::shared_ptr<Slice> slice1,
								const boost::shared_ptr<Slice> slice2)
{
	return *slice1 < *slice2;
}
