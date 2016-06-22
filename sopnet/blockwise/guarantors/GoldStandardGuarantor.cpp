#include <pipeline/Process.h>
#include <pipeline/Value.h>
#include <solvers/LinearConstraints.h>

#include <core/training/GoldStandardExtractor.h>

#include <blockwise/persistence/SegmentDescriptions.h>
#include <blockwise/persistence/exceptions.h>
#include <blockwise/blocks/Cores.h>

#include "GoldStandardGuarantor.h"

logger::LogChannel goldstandardguarantorlog("goldstandardguarantorlog", "[GoldStandardGuarantor] ");

GoldStandardGuarantor::GoldStandardGuarantor(
		const ProjectConfiguration&     projectConfiguration,
		boost::shared_ptr<SegmentStore> gtSegmentStore,
		boost::shared_ptr<SliceStore>   gtSliceStore,
		boost::shared_ptr<StackStore<LabelImage> > gtStackStore,
		boost::shared_ptr<SegmentStore> memSegmentStore,
		boost::shared_ptr<SliceStore>   memSliceStore) :
	SolutionGuarantor(projectConfiguration, memSegmentStore, memSliceStore, 1, false, true, false),
	_gtSegmentStore(gtSegmentStore),
	_gtSliceStore(gtSliceStore),
	_gtStackStore(gtStackStore),
	_memSegmentStore(memSegmentStore),
	_memSliceStore(memSliceStore),
	_blockUtils(projectConfiguration) {

	LOG_DEBUG(goldstandardguarantorlog) << "core size is " << projectConfiguration.getCoreSize() << std::endl;
}

Blocks
GoldStandardGuarantor::guaranteeGoldStandard(const Core& core) {

	LOG_DEBUG(goldstandardguarantorlog)
			<< "requesting gold standard for core ("
			<< core.x() << ", " << core.y() << ", " << core.z()
			<< ")" << std::endl;

	Blocks blocks = _blockUtils.getCoreBlocks(core);
	util::box<unsigned int, 3> bound = _blockUtils.getBoundingBox(blocks);

	pipeline::Value<ImageStack<LabelImage> > labelStack = _gtStackStore->getImageStack(bound);

	boost::shared_ptr<GoldStandardExtractor> extractor = boost::make_shared<GoldStandardExtractor>();

	Blocks missingBlocks;
	boost::shared_ptr<Slices> gtSlices = _gtSliceStore->getSlicesByBlocks(blocks, missingBlocks);
	boost::shared_ptr<SegmentDescriptions> gtSegmentDescriptions = _gtSegmentStore->getSegmentsByBlocks(blocks, missingBlocks, false);
	boost::shared_ptr<Segments> gtSegments = gtSegmentDescriptions->asSegments(*gtSlices);

	boost::shared_ptr<Slices> memSlices = _memSliceStore->getSlicesByBlocks(blocks, missingBlocks);
	boost::shared_ptr<SegmentDescriptions> memSegmentDescriptions = _memSegmentStore->getSegmentsByBlocks(blocks, missingBlocks, false);
	boost::shared_ptr<Segments> memSegments = memSegmentDescriptions->asSegments(*memSlices);
	boost::shared_ptr<ConflictSets> memConflictSets = _memSliceStore->getConflictSetsByBlocks(blocks, missingBlocks);
	SegmentConstraints memExplicitConstraints;

	if (!missingBlocks.empty())
			return missingBlocks;

	boost::shared_ptr<LinearConstraints> constraints = boost::make_shared<LinearConstraints>();

	extractor->setInput("ground truth", labelStack);
	extractor->setInput("ground truth segments", gtSegments);
	extractor->setInput("all segments", memSegments);
	extractor->setInput("all linear constraints", constraints);

	pipeline::Value<LinearObjective> goldStandardObjective = extractor->getOutput("gold standard objective");

	unsigned int i = 0;
	std::map<SegmentHash, double> segmentCosts;
	const std::vector<double>& goldStandardCoeffs = goldStandardObjective->getCoefficients();

	for (boost::shared_ptr<EndSegment> end : memSegments->getEnds()) {

		segmentCosts[hash_value(*end)] = goldStandardCoeffs[i];
		i++;
	}

	for (boost::shared_ptr<ContinuationSegment> continuation : memSegments->getContinuations()) {

		segmentCosts[hash_value(*continuation)] = goldStandardCoeffs[i];
		i++;
	}

	for (boost::shared_ptr<BranchSegment> branch : memSegments->getBranches()) {

		segmentCosts[hash_value(*branch)] = goldStandardCoeffs[i];
		i++;
	}

	SegmentDescriptions costSegmentDescriptions;
	for (const SegmentDescription& segment : *memSegmentDescriptions) {
		SegmentDescription newSegment = segment;
		newSegment.setCost(segmentCosts.at(newSegment.getHash()));
		costSegmentDescriptions.add(newSegment);
	}

	std::vector<SegmentHash> solution = SolutionGuarantor::computeSolution(costSegmentDescriptions, *memConflictSets, memExplicitConstraints);
	std::vector<std::set<SegmentHash> > assemblies = extractAssemblies(solution, costSegmentDescriptions);

	_memSegmentStore->storeSolution(assemblies, core);

	// there are no missing blocks
	return Blocks();
}
