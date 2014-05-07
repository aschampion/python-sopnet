
#include <iostream>
#include <string>
#include <fstream>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <util/Logger.h>

#include <pipeline/all.h>
#include <pipeline/Value.h>

#include <boost/unordered_set.hpp>
#include <util/ProgramOptions.h>
#include <util/exceptions.h>
#include <util/httpclient.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <sopnet/catmaid/django/DjangoBlockManager.h>

using std::cout;
using std::endl;
using namespace logger;

void handleException(boost::exception& e) {

	LOG_ERROR(out) << "[window thread] caught exception: ";

	if (boost::get_error_info<error_message>(e))
		LOG_ERROR(out) << *boost::get_error_info<error_message>(e);

	if (boost::get_error_info<stack_trace>(e))
		LOG_ERROR(out) << *boost::get_error_info<stack_trace>(e);

	LOG_ERROR(out) << std::endl;

	LOG_ERROR(out) << "[window thread] details: " << std::endl
	               << boost::diagnostic_information(e)
	               << std::endl;

	exit(-1);
}

int main(int optionc, char** optionv)
{
	util::ProgramOptions::init(optionc, optionv);
	LogManager::init();
	
	try
	{
		boost::shared_ptr<BlockManager> blockManager =
			DjangoBlockManager::getBlockManager("catmaid:8000", 1, 1);
		util::point3<unsigned int> stackSize = blockManager->stackSize();
		util::point3<unsigned int> blockSize = blockManager->blockSize();
		
		boost::shared_ptr<Block> block0 =
			blockManager->blockAtLocation(util::point3<unsigned int>(0,0,0));
		boost::shared_ptr<Block> blockNull = 
			blockManager->blockAtLocation(stackSize);
		boost::shared_ptr<Block> blockSup = 
			blockManager->blockAtLocation(stackSize - util::point3<unsigned int>(1, 1, 1));
		boost::shared_ptr<Blocks> allBlocks = blockManager->blocksInBox(
			boost::make_shared<Box<> >(util::point3<unsigned int>(0,0,0), stackSize));
		util::point3<unsigned int> maxCoords = blockManager->maximumBlockCoordinates();
		unsigned int n = maxCoords.x * maxCoords.y * maxCoords.z;

		bool ok = allBlocks->length() == n;
		
		LOG_USER(out) << "Got " << allBlocks->length() << " blocks, expected " << n << endl;
		
		LOG_USER(out) << "For stack size " << stackSize << " and block size " << blockSize << ":" <<
			endl;
		LOG_USER(out) << "\tBlock 0: " << *block0 << endl;
		LOG_USER(out) << "\tBlock Sup: " << *blockSup << endl;
		if (blockNull)
		{
			LOG_USER(out) << "\tBlock inf (should be null): " << *blockNull << endl;
			ok = false;
		}
		else
		{
			LOG_USER(out) << "\tBlock inf is correctly null" << std::endl;
		}
		
		foreach (boost::shared_ptr<Block> block, *allBlocks)
		{
			if (! (*block == *blockManager->blockAtCoordinates(block->getCoordinates())))
			{
				ok = false;
				LOG_USER(out) << "Block " << *block << " with coordinates " << block->getCoordinates()
					<< " did not match block returned by the block manager for those coordinates" << endl;
			}
		}
		
		foreach (boost::shared_ptr<Block> block, *allBlocks)
		{
			if (block->getSlicesFlag())
			{
				ok = false;
				LOG_USER(out) << "Block " << *block << " slices flag instantiated to true" << endl;
			}
			block->setSlicesFlag(true);
		}
		
		foreach (boost::shared_ptr<Block> block, *allBlocks)
		{
			if (!block->getSlicesFlag())
			{
				ok = false;
				LOG_USER(out) << "Block " << *block << " slices flag false, previously set true" << endl;
			}
			if (block->getSegmentsFlag())
			{
				ok = false;
				LOG_USER(out) << "Block " << *block << " segments flag instantiated to true" << endl;
			}
			block->setSegmentsFlag(true);
		}
		
		foreach (boost::shared_ptr<Block> block, *allBlocks)
		{
			if (!block->getSegmentsFlag())
			{
				ok = false;
				LOG_USER(out) << "Block " << *block << " segments flag false, previously set true" << endl;
			}
			if (block->getSolutionCostFlag())
			{
				ok = false;
				LOG_USER(out) << "Block " << *block << " solutions flag instantiated to true" << endl;
			}
			block->setSolutionCostFlag(true);
		}

		foreach (boost::shared_ptr<Block> block, *allBlocks)
		{
			if (!block->getSolutionCostFlag())
			{
				ok = false;
				LOG_USER(out) << "Block " << *block << " solutions flag false, previously set true" << endl;
			}
		}

		
		if (ok)
		{
			LOG_USER(out) << "Block manager test passed" << endl;
		}
		else
		{
			LOG_USER(out) << "Block manager test failed" << endl;
		}
		
	}
	catch (Exception& e)
	{
		handleException(e);
	}
}
