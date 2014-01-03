#ifndef BOX_H__
#define BOX_H__

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <pipeline/Data.h>
#include <util/point3.hpp>
#include <util/point.hpp>
#include <util/rect.hpp>

using boost::make_shared;
using util::point3;
using util::point;
using util::rect;

/**
 * Box - a class that represents a 3D rectangular geometry.
 * 
 * This class is designed with packing in mind, and is therefore closed-open. In other words,
 * it contains its minimum bound (returned by location), but does not contain its upper bound,
 * which can be calcuated as location() + size(). It will contain any point that is dimension-wise
 * greater-than-or-equal-to location() and less-than location() + size().
 * 
 * @param T a numerical representation class, which defaults to unsigned int.
 */

template<class T = unsigned int>
class Box : public pipeline::Data
{
public:
	Box() : _location(make_shared<point3<T> >(0, 0, 0)), _size(make_shared<point3<T> >(0, 0, 0)) {}
	
	Box(boost::shared_ptr<point3<T> > location, boost::shared_ptr<point3<T> > size) :
		_location(location), _size(size) {}

	template<class S>
	Box(const Box<S>& box) :
		_location(make_shared<point3<T> >(box._location->x, box._location->y, box._location->z)),
		_size(make_shared<point3<T> >(box._size->x, box._size->y, box._size->z)) {}
	
	/**
	 * Create a Box that contains a rect for a given z-interval. This Box will be one unit
	 * greater in width and depth than the rect, since a rect is strictly closed. This makes
	 * the most sense when T is an integer type.
	 * @param rect the rect to contain
	 * @param zMin the minimum z for the z-interval
	 * @param depth the depth of the z-interval
	 */
	Box(const rect<T>& rect, T zMin, T depth) :
		_location(make_shared<point3<T> >(rect.minX, rect.minY, zMin)), 
	    _size(make_shared<point3<T> >(rect.width() + 1, rect.height() + 1, depth))
			{}

	/**
	 * @returns the point3 representing the minimum bound of this Box.
	 */
	const point3<T> location() const
	{
		return *_location;
	}
	
	/**
	 * @returns the point3 representing the size of this box.
	 * 
	 */
	const point3<T> size() const
	{
		return *_size;
	}
	
	template<typename S>
	bool contains(const point<S>& loc) const
	{
		point<T> location = *_location;
		point<T> size = *_size;
		point<T> point = loc - location;
		
		bool positive = point.x >= 0 && point.y >= 0;
		bool contained = point.x < size.x && point.y < size.y;
		
		return positive && contained;
	}
	
	template<typename S>
	bool contains(const point3<S>& loc) const
	{
		point3<T> point = loc - *_location;

		bool positive = point >= point3<T>();
		bool contained = point < *_size;

		return positive && contained;
	}
	
	template <typename S>
	bool contains(const rect<S>& rect) const
	{
		return contains(point<S>(rect.minX, rect.minY)) &&
			contains(point<S>(rect.maxX, rect.maxY));
	}
	
	template <typename S>
	bool contains(const Box<S>& box) const
	{
		return location() <= box.location() &&
			(location() + size() >= box.location() + box.size());
	}
	
protected:
	boost::shared_ptr<point3<T> > _location;
	boost::shared_ptr<point3<T> > _size;
};


#endif //BOX_H__