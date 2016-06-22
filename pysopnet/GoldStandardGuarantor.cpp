#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <blockwise/guarantors/GoldStandardGuarantor.h>
#include <blockwise/blocks/Cores.h>
#include <util/point.hpp>
#include "GoldStandardGuarantor.h"
#include "logging.h"

namespace python {

Locations
GoldStandardGuarantor::fill(
		const util::point<unsigned int, 3>& request,
		const GoldStandardGuarantorParameters& /*parameters*/,
		const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[GoldStandardGuarantor] fill called for core at " << request << std::endl;

	boost::shared_ptr<SliceStore>   gtSliceStore    = createSliceStore(configuration, GroundTruth);
	boost::shared_ptr<SegmentStore> gtSegmentStore  = createSegmentStore(configuration, GroundTruth);
	boost::shared_ptr<StackStore<LabelImage> > gtStackStore = createStackStore<LabelImage>(configuration, GroundTruth);
	boost::shared_ptr<SliceStore>   memSliceStore   = createSliceStore(configuration, Membrane);
	boost::shared_ptr<SegmentStore> memSegmentStore = createSegmentStore(configuration, Membrane);

	// create the GoldStandardGuarantor process node
	::GoldStandardGuarantor goldStandardGuarantor(
			configuration,
			gtSegmentStore,
			gtSliceStore,
			gtStackStore,
			memSegmentStore,
			memSliceStore);

	LOG_USER(pylog) << "[GoldStandardGuarantor] processing..." << std::endl;

	// find the core that corresponds to the request
	Core core(request.x(), request.y(), request.z());

	// let it do what it was build for
	Blocks missingBlocks = goldStandardGuarantor.guaranteeGoldStandard(core);

	LOG_USER(pylog) << "[GoldStandardGuarantor] collecting missing blocks" << std::endl;

	// collect missing block locations
	Locations missing;
	for (const Block& block : missingBlocks)
		missing.push_back(util::point<unsigned int, 3>(block.x(), block.y(), block.z()));

	return missing;
}

} // namespace python
