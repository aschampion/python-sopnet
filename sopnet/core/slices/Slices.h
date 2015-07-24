#ifndef CELLTRACKER_CELLS_H__
#define CELLTRACKER_CELLS_H__

#include <boost/shared_ptr.hpp>

#include <external/nanoflann/nanoflann.hpp>

#include <imageprocessing/ConnectedComponent.h>
#include <pipeline/all.h>
#include "Slice.h"

typedef std::map<unsigned int, std::vector<unsigned int> > conflicts_type;

/**
 * An adaptor class to use std::vector<boost::shared_ptr<Slice> > in a
 * nanoflann kd-tree.
 */
class SliceVectorAdaptor {

	typedef std::vector<boost::shared_ptr<Slice> > slices_type;

public:

	/**
	 * Create a new adaptor.
	 */
	template <typename Iterator>
	SliceVectorAdaptor(Iterator begin, Iterator end) :
		_slices(begin, end) {}

	/**
	 * Nanoflann access interface. Gets the number of data points.
	 */
	size_t kdtree_get_point_count() const { return _slices.size(); }

	/**
	 * Nanoflann access interface. Gets the distance between two data points.
	 */
	inline double kdtree_distance(const double *p1, const size_t index_p2, size_t) const {

		double d0 = p1[0] - _slices[index_p2]->getComponent()->getCenter().x();
		double d1 = p1[1] - _slices[index_p2]->getComponent()->getCenter().y();

		return d0*d0 + d1*d1;
	}

	/**
	 * Nanoflann access interface. Get the 'dim'th component of the 'index'th
	 * data point.
	 */
	inline double kdtree_get_pt(const size_t index, int dim) const {

		if (dim == 0)
			return _slices[index]->getComponent()->getCenter().x();
		else if (dim == 1)
			return _slices[index]->getComponent()->getCenter().y();
		else return 0;
	}

	/**
	 * Nanoflann access interface. Computes a bounding box for the data or
	 * returns false.
	 */
	template <class BBox>
	bool kdtree_get_bbox(BBox&) const { return false; }

	/**
	 * Get the slice with the given index, as returned by radiusSearch on the kd 
	 * tree.
	 */
	boost::shared_ptr<Slice> operator[](unsigned int i) { return _slices[i]; }

private:

	// a reference to the data to sort into the kd-tree
	const slices_type _slices;
};

/**
 * A collection of slices.
 */
class Slices : public pipeline::Data {

	// nanoflann kd-tree type
	typedef nanoflann::KDTreeSingleIndexAdaptor<
			nanoflann::L2_Simple_Adaptor<double, SliceVectorAdaptor>,
			SliceVectorAdaptor,
			2>
			SliceKdTree;

public:

	struct SliceComparator {
		bool operator()(boost::shared_ptr<Slice> a, boost::shared_ptr<Slice> b) {
			return a->hashValue() < b->hashValue();
		}
	};

	typedef std::set<boost::shared_ptr<Slice>, SliceComparator> slices_type;

	typedef slices_type::iterator       iterator;

	typedef slices_type::const_iterator const_iterator;

	/**
	 * Create a new set of slices.
	 */
	Slices();

	/**
	 * Copy constructor.
	 */
	Slices(const Slices& other);

	~Slices();

	/**
	 * Assignment operator.
	 */
	Slices& operator=(const Slices& other);

	/**
	 * Remove all slices.
	 */
	void clear();

	/**
	 * Add a single slice to this set of slices.
	 */
	void add(boost::shared_ptr<Slice> slice);

	/**
	 * Add a set of slices to this set of slices.
	 */
	void addAll(const Slices& slices);

	/**
	 * Remove the given slice.
	 */
	void remove(boost::shared_ptr<Slice> slice);

	/**
	 * Add information about conflicting slices, e.g., slices that are
	 * overlapping in space.
	 *
	 * @param conflicting A vector of slice ids that are mutually in conflict.
	 */
	template <typename Collection>
	void addConflicts(const Collection& conflicts) {

		for (unsigned int id : conflicts) {

			_conflicts[id].reserve(_conflicts[id].size() + conflicts.size() - 1);

			for (unsigned int otherId : conflicts)
				if (id != otherId)
					_conflicts[id].push_back(otherId);
		}
	}
	
	/**
	 * Copy the conflicts from another Slices
	 * @param slices a Slices object, from which conflict info will be copied.
	 */
	void addConflictsFromSlices(const Slices& slices);
	
	/**
	 * Set the conflicts for a single slice.
	 * 
	 * @param id the id for the slice whose conflicts are to be set.
	 * @param conflicts a vector containing the ids for conflicting Slice's. 
	 */
	void setConflicts(unsigned int id, std::vector<unsigned int> conflicts);

	/**
	 * Get the conflicts for a single slice.
	 * 
	 * @param id the id for the slice whose conflicts are desired.
	 * @return a vector containing the ids of Slice's conflicting with the given Slice.
	 */
	std::vector<unsigned int> getConflicts(unsigned int id);
	
	/**
	 * Check, whether to slices (given by their id) are in conflict.
	 */
	inline bool areConflicting(unsigned int id1, unsigned int id2) {

		// If we don't have any information about slice id1,
		// we assume that there is no conflict.
		if (!_conflicts.count(id1))
			return false;

		for (unsigned int conflictId : _conflicts[id1])
			if (conflictId == id2)
				return true;

		return false;
	}

	const const_iterator begin() const { return _slices.begin(); }

	iterator begin() { return _slices.begin(); }

	const const_iterator end() const { return _slices.end(); }

	iterator end() { return _slices.end(); }

	unsigned int size() const { return _slices.size(); }

	/**
	 * Find all slices within distance to the given center.
	 */
	std::vector<boost::shared_ptr<Slice> > find(const util::point<double, 2>& center, double distance);

	/**
	 * Move all slices in 2D.
	 */
	void translate(const util::point<int, 2>& offset);

private:

	// the slices
	slices_type _slices;

	// map from ids of slices to all ids of conflicting slices
	conflicts_type _conflicts;

	// nanoflann vector adaptor
	SliceVectorAdaptor* _adaptor;

	// a kd-tree, which is created on-demand
	SliceKdTree* _kdTree;

	// indicate that the tree has to be (re)build
	bool _kdTreeDirty;
};

#endif // CELLTRACKER_CELLS_H__
