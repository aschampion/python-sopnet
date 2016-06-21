#ifndef SOPNET_GOLD_STANDARD_EXTRACTOR_H__
#define SOPNET_GOLD_STANDARD_EXTRACTOR_H__

#include <pipeline/SimpleProcessNode.h>
#include <pipeline/Process.h>
#include <util/point.hpp>
#include <imageprocessing/ImageStack.h>
#include <sopnet/core/segments/Segments.h>
#include <sopnet/core/inference/Reconstructor.h>
#include <solvers/LinearConstraints.h>

class GoldStandardExtractor : public pipeline::SimpleProcessNode<> {

public:

	GoldStandardExtractor();

private:

	void updateOutputs();

	pipeline::Input<ImageStack<LabelImage> > _groundTruth;
	pipeline::Input<Segments>                _groundTruthSegments;
	pipeline::Input<Segments>                _allSegments;
	pipeline::Input<LinearConstraints>       _allLinearConstraints;

	pipeline::Process<Reconstructor>      _reconstructor;
	pipeline::Process<ObjectiveGenerator> _objectiveGenerator;
};

#endif // SOPNET_GOLD_STANDARD_EXTRACTOR_H__

