#include "BlockManager.h"
#include <util/Logger.h>
#include <boost/make_shared.hpp>
#include <sopnet/block/Box.h>
#include <sopnet/block/Blocks.h>
#include <sopnet/block/Cores.h>

logger::LogChannel blockmanagerlog("blockmanagerlog", "[BlockManager] ");


/**
 * Creates an instance of a basic implementation of BlockManager, which exists only locally.
 */
BlockManager::BlockManager(const point3<unsigned int> stackSize,
							const point3<unsigned int> blockSize,
							const point3<unsigned int> coreSizeInBlocks) :
							_stackSize(stackSize), _blockSize(blockSize),
							_coreSizeInBlocks(coreSizeInBlocks)
{
    _maxBlockCoordinates = (stackSize + blockSize - point3<unsigned int>(1u, 1u, 1u)) / blockSize;
	_coreSize = util::point3<unsigned int>(coreSizeInBlocks.x * blockSize.x,
										   coreSizeInBlocks.y * blockSize.y,
										   coreSizeInBlocks.z * blockSize.z);
	_maxCoreCoordinates = (stackSize + _coreSize - point3<unsigned int>(1u, 1u, 1u)) / _coreSize;

	LOG_DEBUG(blockmanagerlog) << "Stack size: " << stackSize << ", block size: " << blockSize <<
		std::endl;
	LOG_DEBUG(blockmanagerlog) << "Maximum block coordinates: " << _maxBlockCoordinates <<
		std::endl;
	LOG_DEBUG(blockmanagerlog) << "Maximum core coordinates: " << _maxCoreCoordinates <<
		std::endl;
}

boost::shared_ptr<Block>
BlockManager::blockAtLocation(unsigned int x, unsigned int y, unsigned int z)
{
	return blockAtLocation(util::point3<unsigned int>(x, y, z));
}

boost::shared_ptr<Block>
BlockManager::blockAtLocation(const point3<unsigned int>& location)
{
	if (isValidLocation(location))
	{
		point3<unsigned int> blockCoordinates = location / _blockSize;
		LOG_DEBUG(blockmanagerlog) << "Converted location " << location << " to coordinates " <<
			blockCoordinates << std::endl;
		return blockAtCoordinates(blockCoordinates);
	}
	else
	{
		return boost::shared_ptr<Block>();
	}
}

boost::shared_ptr<Block>
BlockManager::blockAtOffset(const Block& block, const point3<int>& offset)
{
	point3<int> signedBlockCoordinates = offset + (block.location() / _blockSize);
	point3<unsigned int> blockCoordinates = signedBlockCoordinates;
	
	if (signedBlockCoordinates >= point3<int>(0,0,0) && blockCoordinates < _maxBlockCoordinates)
	{
		return blockAtCoordinates(blockCoordinates);
	}
	else
	{
		LOG_ALL(blockmanagerlog) << "Invalid block coordinates: " << offset << std::endl;
		LOG_ALL(blockmanagerlog) << "Max block coordinates: " << _maxBlockCoordinates << std::endl;
		return boost::shared_ptr<Block>();
	}
}

const util::point3<unsigned int>&
BlockManager::blockSize() const
{
	return _blockSize;
}

const util::point3<unsigned int>&
BlockManager::stackSize() const
{
	return _stackSize;
}

boost::shared_ptr<Cores>
BlockManager::coresInBox(const boost::shared_ptr<Box<> > box)
{
	util::point3<unsigned int> corner = box->location();
	util::point3<unsigned int> size = box->size();
	boost::shared_ptr<Cores> cores = boost::make_shared<Cores>();
	
	for (unsigned int z = corner.z; z - corner.z < size.z; z += _coreSize.z)
	{
		for (unsigned int y = corner.y; y - corner.y < size.y; y += _coreSize.y)
		{
			for (unsigned int x = corner.x; x - corner.x < size.x; x += _coreSize.x)
			{
				util::point3<unsigned int> coords = point3<unsigned int>(x, y, z) / _coreSize;
				boost::shared_ptr<Core> core = coreAtCoordinates(coords);
				cores->add(core);
			}
		}
	}

	return cores;
}

boost::shared_ptr<Blocks>
BlockManager::blocksInBox(const boost::shared_ptr< Box<unsigned int> >& box)
{
	util::point3<unsigned int> corner = box->location();
	util::point3<unsigned int> size = box->size();
	boost::shared_ptr<Blocks> blocks = boost::make_shared<Blocks>();
	for (unsigned int z = corner.z; z - corner.z < size.z; z += blockSize().z)
	{
		for (unsigned int y = corner.y; y - corner.y < size.y; y += blockSize().y)
		{
			for (unsigned int x = corner.x; x - corner.x < size.x; x += blockSize().x)
			{
				util::point3<unsigned int> coords = point3<unsigned int>(x, y, z) / blockSize();
				boost::shared_ptr<Block> block = blockAtCoordinates(coords);
				if (block)
				{
					blocks->add(block);
				}
			}
		}
	}

	return blocks;
}

const util::point3<unsigned int>&
BlockManager::maximumBlockCoordinates()
{
	return _maxBlockCoordinates;
}

const util::point3<unsigned int>&
BlockManager::maximumCoreCoordinates()
{
	return _maxCoreCoordinates;
}

bool
BlockManager::isValidBlockCoordinates(const util::point3< unsigned int >& coords) const
{
	return coords < _maxBlockCoordinates;
}

bool BlockManager::isValidLocation(const util::point3< unsigned int >& loc) const
{
	return loc < _stackSize;
}

boost::shared_ptr<Core>
BlockManager::coreAtLocation(unsigned int x, unsigned int y, unsigned int z)
{
	return coreAtLocation(util::point3<unsigned int>(x, y, z));
}

boost::shared_ptr<Core>
BlockManager::coreAtLocation(const util::point3<unsigned int> location)
{
	if (isValidLocation(location))
	{
		point3<unsigned int> coreCoordinates = location / _coreSize;
		return coreAtCoordinates(coreCoordinates);
	}
	else
	{
		return boost::shared_ptr<Core>();
	}
}

const util::point3<unsigned int>&
BlockManager::coreSize()
{
	return _coreSize;
}

const util::point3<unsigned int>&
BlockManager::coreSizeInBlocks()
{
	return _coreSizeInBlocks;
}

bool
BlockManager::isValidCoreCoordinates(const util::point3<unsigned int>& coords) const
{
	return coords < _maxCoreCoordinates;
}



bool
BlockManager::isValidZ(unsigned int z)
{
	return z < _stackSize.z;
}

bool
BlockManager::isUpperBound(unsigned int z)
{
	return z == _stackSize.z - 1;
}
