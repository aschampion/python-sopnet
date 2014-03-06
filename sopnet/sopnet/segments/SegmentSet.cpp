#include "SegmentSet.h"

void
SegmentSet::add(const boost::shared_ptr<Segment> segment)
{
	_segments.insert(segment);
}

bool
SegmentSet::contains(const boost::shared_ptr<Segment> segment) const
{
	if (_segments.count(segment))
	{
		return true;
	}
	else
	{
		return false;
	}
}

boost::shared_ptr<Segment>
SegmentSet::find(const boost::shared_ptr<Segment> segment) const
{
	if (contains(segment))
	{
		return *_segments.find(segment);
	}
	
	return boost::shared_ptr<Segment>();
}
