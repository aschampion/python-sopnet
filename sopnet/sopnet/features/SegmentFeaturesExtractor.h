#ifndef SOPNET_SEGMENT_FEATURES_EXTRACTOR_H__
#define SOPNET_SEGMENT_FEATURES_EXTRACTOR_H__

#include <pipeline/all.h>
#include <imageprocessing/ImageStack.h>
#include <sopnet/segments/Segments.h>
#include <util/point3.hpp>
#include "Features.h"

// forward declaration
class GeometryFeatureExtractor;
class HistogramFeatureExtractor;

class SegmentFeaturesExtractor : public pipeline::ProcessNode {

public:

	SegmentFeaturesExtractor();

private:

	class FeaturesAssembler : public pipeline::SimpleProcessNode<> {

	public:

		FeaturesAssembler();

	private:

		void updateOutputs();

		pipeline::Inputs<Features> _features;

		pipeline::Output<Features> _allFeatures;
	};

	void onInputSet(const pipeline::InputSetBase& signal);
	
	void onOffsetSet(const pipeline::InputSetBase&);

	pipeline::Input<Segments> _segments;

	pipeline::Input<ImageStack> _rawSections;
	
	pipeline::Input<util::point3<unsigned int> > _cropOffset;

	boost::shared_ptr<GeometryFeatureExtractor>  _geometryFeatureExtractor;

	boost::shared_ptr<HistogramFeatureExtractor> _histogramFeatureExtractor;

	boost::shared_ptr<FeaturesAssembler> _featuresAssembler;
};

#endif // SOPNET_SEGMENT_FEATURES_EXTRACTOR_H__

