#ifndef IALLOCATOR_HPP
#define IALLOCATOR_HPP

/*************************************************************************
 * The IAllocator class handles allocation of memory on a given block of
 * contiguous data. The allocator uses a simple 'first-fit' algorithm.
*************************************************************************/

#include <stdint.h>
#include <set>

struct IAllocatorUsedBlock
{
	uint8_t* 	m_StartPtr;
	unsigned int 	m_SizeInBytes;

	IAllocatorUsedBlock (uint8_t* startPtr, unsigned int sizeInBytes);
	~IAllocatorUsedBlock();

	bool operator< (const IAllocatorUsedBlock& other) const;
};

class IAllocator
{
	public:
		IAllocator (uint8_t* startPtr, unsigned int sizeInBytes);
		~IAllocator();

		template <typename T, typename... A>
		T* allocate(A... constructorArgs)
		{
			const unsigned int sizeOfType = sizeof( T );

			// look for size in between two blocks that fits sizeOfType
			for ( auto usedBlockIt = m_UsedBlocks.begin(); usedBlockIt != m_UsedBlocks.end(); usedBlockIt++ )
			{
				const IAllocatorUsedBlock& usedBlock = *usedBlockIt;
				const auto nextUsedBlockIt = std::next( usedBlockIt );
				if ( nextUsedBlockIt != m_UsedBlocks.end() )
				{
					const uint8_t* const nextUsedBlockStartPtr = nextUsedBlockIt->m_StartPtr;
					const unsigned int spaceBetween = nextUsedBlockStartPtr
										- ( usedBlock.m_StartPtr + usedBlock.m_SizeInBytes );

					// if the data fits in the space between these blocks place it there, add a new used block to the list,
					// and return the pointer
					if ( spaceBetween >= sizeOfType )
					{
						uint8_t* const startPtr = usedBlock.m_StartPtr + usedBlock.m_SizeInBytes;
						IAllocatorUsedBlock newBlock( startPtr, sizeOfType );

						m_UsedBlocks.insert( newBlock );

						return new ( startPtr ) T( constructorArgs... );
					}
				}
			}

			return nullptr;
		}

		template <typename T>
		bool free (T* dataToFreePtr) // returns true if successful, false otherwise
		{
			const uint8_t* const dataToFreeUIntPtr = reinterpret_cast<const uint8_t* const>( dataToFreePtr );

			// ensure we don't remove our first and last block added in constructor for comparison
			if ( dataToFreeUIntPtr != m_StartPtr && dataToFreeUIntPtr != m_StartPtr + m_SizeInBytes )
			{
				// find the used block that this data points to
				for ( auto usedBlockIt = m_UsedBlocks.begin(); usedBlockIt != m_UsedBlocks.end(); usedBlockIt++ )
				{
					// if found, remove the block from the used block list
					if ( dataToFreeUIntPtr == usedBlockIt->m_StartPtr )
					{
						m_UsedBlocks.erase( usedBlockIt );
						return true;
					}
				}
			}

			return false;
		}

	private:
		uint8_t* 			m_StartPtr;
		unsigned int 			m_SizeInBytes;

		std::set<IAllocatorUsedBlock> 	m_UsedBlocks;
};

#endif // IALLOCATOR_HPP
