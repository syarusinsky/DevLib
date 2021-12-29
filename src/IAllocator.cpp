#include "IAllocator.hpp"

IAllocatorUsedBlock::IAllocatorUsedBlock (uint8_t* startPtr, unsigned int sizeInBytes) :
	m_StartPtr( startPtr ),
	m_SizeInBytes( sizeInBytes )
{
}

IAllocatorUsedBlock::~IAllocatorUsedBlock()
{
}

bool IAllocatorUsedBlock::operator< (const IAllocatorUsedBlock& other) const
{
	if ( m_StartPtr == other.m_StartPtr )
	{
		if ( m_SizeInBytes < other.m_SizeInBytes )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else if ( m_StartPtr < other.m_StartPtr )
	{
		return true;
	}
	else
	{
		return false;
	}
}

IAllocator::IAllocator (uint8_t* startPtr, unsigned int sizeInBytes) :
	m_StartPtr( startPtr ),
	m_SizeInBytes( sizeInBytes ),
	m_UsedBlocks()
{
	// add first and last block single byte for comparison during allocation
	m_UsedBlocks.insert( IAllocatorUsedBlock(startPtr, 0) );
	m_UsedBlocks.insert( IAllocatorUsedBlock(startPtr + sizeInBytes, 0) );
}

IAllocator::~IAllocator()
{
}
