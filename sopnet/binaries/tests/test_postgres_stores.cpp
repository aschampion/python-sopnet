#include <iostream>
#include <blockwise/ProjectConfiguration.h>
#include <blockwise/persistence/postgresql/PostgreSqlSegmentStore.h>
#include <blockwise/persistence/postgresql/PostgreSqlSliceStore.h>
#include <util/exceptions.h>
#include <util/box.hpp>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/point.hpp>
#include <slices/ConflictSets.h>

util::ProgramOption optionStackId(
		util::_long_name        = "stack",
		util::_short_name       = "s",
		util::_description_text = "The Sopnet raw stack ID.",
		util::_default_value	  = 2);

util::ProgramOption optionComponentDir(
		util::_long_name        = "compdir",
		util::_short_name       = "c",
		util::_description_text = "Component storage directory",
		util::_default_value	  = "/tmp/catsop");

util::ProgramOption optionPGHost(
		util::_long_name        = "pghost",
		util::_short_name       = "H",
		util::_description_text = "The PostgreSQL host",
		util::_default_value	  = "");

util::ProgramOption optionPGUser(
		util::_long_name        = "pguser",
		util::_short_name       = "U",
		util::_description_text = "The PostgreSQL user",
		util::_default_value	  = "catsop_user");

util::ProgramOption optionPGPassword(
		util::_long_name        = "pgpassword",
		util::_short_name       = "P",
		util::_description_text = "The PostgreSQL password",
		util::_default_value	  = "catsop_janelia_test");

util::ProgramOption optionPGDatabase(
		util::_long_name        = "pgdatabase",
		util::_short_name       = "D",
		util::_description_text = "The PostgreSQL database",
		util::_default_value	  = "catsop");

boost::shared_ptr<Slice>
createSlice(unsigned int pixelEntry) {

	boost::shared_ptr<ConnectedComponent::pixel_list_type> pixelList =
			boost::make_shared<ConnectedComponent::pixel_list_type>();

	pixelList->add(util::point<unsigned int, 2>(pixelEntry, pixelEntry));
	pixelList->add(util::point<unsigned int, 2>(pixelEntry + 1, pixelEntry + 1));

	boost::shared_ptr<ConnectedComponent> cc = boost::make_shared<ConnectedComponent>(
			std::array<char, 8>(),
			pixelList,
			pixelList->begin(),
			pixelList->end());

	return boost::make_shared<Slice>(0, 0, cc);
}

int main(int argc, char** argv)
{
	try {
		// init command line parser
		util::ProgramOptions::init(argc, argv);

		int stack_id = optionStackId.as<int>();
		std::string comp_dir = optionComponentDir.as<std::string>();
		std::string pg_host = optionPGHost.as<std::string>();
		std::string pg_user = optionPGUser.as<std::string>();
		std::string pg_pass = optionPGPassword.as<std::string>();
		std::string pg_dbase = optionPGDatabase.as<std::string>();


		std::cout << "Testing PostgreSQL stores with stack ID " << stack_id << std::endl;

		// init logger
		logger::LogManager::init();
		logger::LogManager::setGlobalLogLevel(logger::Debug);

		// create new project configuration
		ProjectConfiguration pc;
		pc.setBackendType(ProjectConfiguration::PostgreSql);
		StackDescription stack;
		stack.id = stack_id;
		pc.setCatmaidStack(Raw, stack);
		pc.setComponentDirectory(comp_dir);
		pc.setPostgreSqlHost(pg_host);
		pc.setPostgreSqlUser(pg_user);
		pc.setPostgreSqlPassword(pg_pass);
		pc.setPostgreSqlDatabase(pg_dbase);

		PostgreSqlSliceStore sliceStore(pc, Membrane);

		// Add first set of slices
		boost::shared_ptr<Slice> slice1 = createSlice(0);
		boost::shared_ptr<Slice> slice2 = createSlice(1);
		boost::shared_ptr<Slice> slice3 = createSlice(2);

		Slices slices = Slices();
		slices.add(slice1);
		slices.add(slice2);
		slices.add(slice3);

		Block block(0, 0, 0);
		sliceStore.associateSlicesToBlock(slices, block);

		Blocks blocks;
		blocks.add(block);
		Blocks missingBlocks;

		boost::shared_ptr<Slices> retrievedSlices =
				sliceStore.getSlicesByBlocks(blocks, missingBlocks);

		// Create conflict set where each slice
		ConflictSet conflictSet1;
		conflictSet1.addSlice(slice1->hashValue());
		conflictSet1.addSlice(slice2->hashValue());
		conflictSet1.addSlice(slice3->hashValue());

		ConflictSets conflictSets;
		conflictSets.add(conflictSet1);

		sliceStore.associateConflictSetsToBlock(conflictSets, block);
		boost::shared_ptr<ConflictSets> retrievedConflictSets =
				sliceStore.getConflictSetsByBlocks(blocks, missingBlocks);
		for (const ConflictSet& cs : *retrievedConflictSets) {
			std::cout << "ConflictSet hash: " << hash_value(cs);

			for (const SliceHash& sh : cs.getSlices()) {
				std::cout << " Slice hash: " << sh;
			}

			std::cout << std::endl;
		}

		PostgreSqlSegmentStore segmentStore(pc, Membrane);
		util::box<unsigned int, 2> segmentBounds(0, 0, 0, 0);
		std::vector<double> segmentFeatures;
		segmentFeatures.push_back(0.0);
		segmentFeatures.push_back(1.0);
		segmentFeatures.push_back(2.0);
		SegmentDescription segment(0, segmentBounds);
		segment.addLeftSlice(slice1->hashValue());
		segment.addRightSlice(slice2->hashValue());
		segment.setFeatures(segmentFeatures);

		boost::shared_ptr<SegmentDescriptions> segments = boost::make_shared<SegmentDescriptions>();
		segments->add(segment);

		segmentStore.associateSegmentsToBlock(*segments, block);

		boost::shared_ptr<SegmentDescriptions> retrievedSegments =
				segmentStore.getSegmentsByBlocks(blocks, missingBlocks, false);

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}
