#include "SegmentDescription.h"
#include <util/foreach.h>
#include <segments/SegmentHash.h>

std::size_t
SegmentDescription::getHash() const {

	if (_hashDirty) {

		std::vector<std::size_t> sliceHashes;

		foreach (std::size_t hash, _leftSliceHashes)
			sliceHashes.push_back(hash);
		foreach (std::size_t hash, _rightSliceHashes)
			sliceHashes.push_back(hash);

		_hash = hash_value(sliceHashes);
		_hashDirty = false;
	}

	return _hash;
}
