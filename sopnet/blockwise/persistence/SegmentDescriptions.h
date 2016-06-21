#ifndef SOPNET_BLOCKWISE_PERSISTENCE_SEGMENT_DESCRIPTIONS_H__
#define SOPNET_BLOCKWISE_PERSISTENCE_SEGMENT_DESCRIPTIONS_H__

#include <vector>
#include <util/exceptions.h>
#include <sopnet/core/segments/Segments.h>
#include <sopnet/core/slices/Slices.h>
#include "SegmentDescription.h"

class SegmentDescriptions {

public:

	struct SegmentDescriptionComparator {
		bool operator()(const SegmentDescription& a, const SegmentDescription& b) {
			return a.getHash() < b.getHash();
		}
	};

	typedef std::set<SegmentDescription, SegmentDescriptionComparator> segments_type;
	typedef segments_type::iterator                                    iterator;
	typedef segments_type::const_iterator                              const_iterator;

	void add(const SegmentDescription& segment) { _segments.insert(segment); }

	unsigned int size() const { return _segments.size(); }

	iterator begin() { return _segments.begin(); }
	iterator end() { return _segments.end(); }
	const_iterator begin() const { return _segments.begin(); }
	const_iterator end() const { return _segments.end(); }

	boost::shared_ptr<Segments> asSegments(const Slices& slices) const {

		boost::shared_ptr<Segments> segments = boost::make_shared<Segments>();

		std::map<SliceHash, boost::shared_ptr<Slice> > sliceHashMap;

		for (boost::shared_ptr<Slice> slice : slices) {
			sliceHashMap[slice->hashValue()] = slice;
		}

		for (const SegmentDescription& segment : *this) {

			boost::shared_ptr<Segment> fullSegment = segment.asSegment(sliceHashMap);

			if (segment.getHash() != hash_value(*fullSegment)) {
				UTIL_THROW_EXCEPTION(Exception,
						"A hash changed when converting a SegmentDescription to a Segment");
			}

			segments->add(fullSegment);
		}

		return segments;
	}

private:

	segments_type _segments;
};

#endif // SOPNET_BLOCKWISE_PERSISTENCE_SEGMENT_DESCRIPTION_H__

