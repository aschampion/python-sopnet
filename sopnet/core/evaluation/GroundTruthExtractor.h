#ifndef SOPNET_GROUND_TRUTH_EXTRACTOR_H__
#define SOPNET_GROUND_TRUTH_EXTRACTOR_H__

#include <vector>

#include <pipeline/SimpleProcessNode.h>
#include <imageprocessing/ImageStack.h>
#include <sopnet/core/slices/Slices.h>
#include <sopnet/core/segments/Segments.h>
#include <sopnet/core/features/Overlap.h>

extern util::ProgramOption optionGroundTruthFromSkeletons;

class GroundTruthExtractor : public pipeline::SimpleProcessNode<> {

public:

	/**
	 * Create a ground truth extractor.
	 *
	 * @param endSegmentsOnly
	 *              Extract only end segments, no continuations. Each slice will 
	 *              be a "neuron".
	 */
	GroundTruthExtractor(bool endSegmentsOnly = false);

private:

	/**
	 * Sorts continuations by source-target overlap descending.
	 */
	struct ContinuationComparator {

		ContinuationComparator() :
			overlap(false, false) {}

		bool operator()(const ContinuationSegment& a, const ContinuationSegment& b) {

			double overlapA = overlap(*a.getSourceSlice(), *a.getTargetSlice());
			double overlapB = overlap(*b.getSourceSlice(), *b.getTargetSlice());

			// larger overlap goes to front
			if (overlapA != overlapB)
				return overlapA > overlapB;

			util::point<double, 2> diffA = a.getTargetSlice()->getComponent()->getCenter() - a.getSourceSlice()->getComponent()->getCenter();
			util::point<double, 2> diffB = b.getTargetSlice()->getComponent()->getCenter() - b.getSourceSlice()->getComponent()->getCenter();

			double distanceA = diffA.x()*diffA.x() + diffA.y()*diffA.y();
			double distanceB = diffB.x()*diffB.x() + diffB.y()*diffB.y();

			// smaller displacement goes to front
			return distanceA < distanceB;
		}

		Overlap overlap;
	};

	void updateOutputs();

	// extract all slices of each ground-truth section
	std::vector<Slices> extractSlices(int firstSection, int lastSection);

	std::map<LabelImage::value_type, std::vector<ContinuationSegment> > extractContinuations(const std::vector<Slices>& slices);

	// find a minimal spanning segment tree for each set of slices with the same 
	// id
	Segments findMinimalTrees(const std::vector<Slices>& slices);

	// find on tree of segments per connected component of label
	void findLabelTree(
			LabelImage::value_type label,
			std::vector<ContinuationSegment>& continuations,
			std::map<unsigned int, unsigned int>& linksLeft,
			std::map<unsigned int, unsigned int>& linksRight,
			Segments& segments);

	// the ground truth images
	pipeline::Input<ImageStack<LabelImage> > _groundTruthSections;

	// continuation and end segments of the ground-truth
	pipeline::Output<Segments> _groundTruthSegments;

	bool _addIntensityBoundaries;

	bool _endSegmentsOnly;
};

#endif // SOPNET_GROUND_TRUTH_EXTRACTOR_H__

