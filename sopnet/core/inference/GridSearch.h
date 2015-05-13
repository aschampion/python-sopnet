#ifndef SOPNET_INFERENCE_GRID_SEARCH_H__
#define SOPNET_INFERENCE_GRID_SEARCH_H__

#include <pipeline/all.h>
#include "PriorCostFunctionParameters.h"
#include "SegmentationCostFunctionParameters.h"

class GridSearch : public pipeline::SimpleProcessNode<> {

public:

	GridSearch();

	/**
	 * Set the next parameters in the grid search.
	 *
	 * @return false, if there are no more paramter values to set.
	 */
	bool next();

	/**
	 * Get the current configuration as a string.
	 *
	 * @return A string representing the current configuration.
	 */
	std::string currentParameters() {

		return
				std::string("end_") + boost::lexical_cast<std::string>(_priorCostFunctionParameters->priorEnd) +
				std::string("__") +
				std::string("continuation_") + boost::lexical_cast<std::string>(_priorCostFunctionParameters->priorContinuation) +
				std::string("__") +
				std::string("branch__") + boost::lexical_cast<std::string>(_priorCostFunctionParameters->priorBranch) +
				std::string("__") +
				std::string("segweight_") + boost::lexical_cast<std::string>(_segmentationCostFunctionParameters->weight) +
				std::string("__") +
				std::string("seppotts_") + boost::lexical_cast<std::string>(_segmentationCostFunctionParameters->weightPotts) +
				std::string("__") +
				std::string("segfore_") + boost::lexical_cast<std::string>(_segmentationCostFunctionParameters->priorForeground);
	}

private:

	void updateOutputs() {}

	pipeline::Output<PriorCostFunctionParameters>        _priorCostFunctionParameters;
	pipeline::Output<SegmentationCostFunctionParameters> _segmentationCostFunctionParameters;
};

#endif // SOPNET_INFERENCE_GRID_SEARCH_H__

