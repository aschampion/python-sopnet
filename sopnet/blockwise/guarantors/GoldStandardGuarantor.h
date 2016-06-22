#ifndef GOLD_STANDARD_GUARANTOR_H__
#define GOLD_STANDARD_GUARANTOR_H__

#include <boost/shared_ptr.hpp>

#include <blockwise/ProjectConfiguration.h>
#include <blockwise/persistence/SegmentStore.h>
#include <blockwise/persistence/SliceStore.h>
#include <blockwise/persistence/StackStore.h>
#include <blockwise/blocks/BlockUtils.h>
#include <blockwise/blocks/Core.h>

#include <segments/SegmentHash.h>

#include "SolutionGuarantor.h"

class GoldStandardGuarantor :
		private SolutionGuarantor {

public:

	/**
	 * Create a new GoldStandardGuarantor using the given stores.
	 *
	 * @param projectConfiguration
	 *              The ProjectConfiguration used to configure Block and Core
	 *              parameters.
	 *
	 * @param gtSegmentStore
	 *              The SegmentStore to retrieve segments extracted from ground
	 *              truth labels.
	 *
	 * @param gtSliceStore
	 *              The slice store to retrieve slices extracted from ground
	 *              truth labels.
	 *
	 * @param gtStackStore
	 *        	    The stack store to retrieve the ground truth labels.
	 *
	 * @param memSegmentStore
	 *              The SegmentStore to retrieve segments extracted from
	 *              membrane prediction and to store the gold standard solution.
	 *
	 * @param memSliceStore
	 *              The slice store to retrieve slices extracted from membrane
	 *              prediction labels.
	 */
	GoldStandardGuarantor(
			const ProjectConfiguration&     projectConfiguration,
			boost::shared_ptr<SegmentStore> gtSegmentStore,
			boost::shared_ptr<SliceStore>   gtSliceStore,
			boost::shared_ptr<StackStore<LabelImage> > gtStackStore,
			boost::shared_ptr<SegmentStore> memSegmentStore,
			boost::shared_ptr<SliceStore>   memSliceStore);
	/**
	 * Get the gold standard solution for the given core from a ground truth.
	 *
	 * @param core
	 *              The core to extract the gold standard for.
	 */
	Blocks guaranteeGoldStandard(const Core& core);

private:

	boost::shared_ptr<SegmentStore>            _gtSegmentStore;
	boost::shared_ptr<SliceStore>              _gtSliceStore;
	boost::shared_ptr<StackStore<LabelImage> > _gtStackStore;
	boost::shared_ptr<SegmentStore>            _memSegmentStore;
	boost::shared_ptr<SliceStore>              _memSliceStore;

	BlockUtils _blockUtils;
};

#endif //GOLD_STANDARD_GUARANTOR_H__
