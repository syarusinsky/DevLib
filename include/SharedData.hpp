#ifndef SHAREDDATA_HPP
#define SHAREDDATA_HPP

#include "IAllocator.hpp"

/*************************************************************************
 * The SharedData class is basically just a shared pointer. It's mostly
 * used for memory that needs to be allocated in IStorageMedia but
 * deleted outside of it.
*************************************************************************/

template <typename T>
class SharedData
{
	public:
		static unsigned int m_TotalBytesAllocated;

		SharedData (const SharedData& other)
		{
			// if the underlying data is different, we need to first decrease the ref count of the previous data and delete if necessary
			this->decrementAndDeletePreviousUnderlyingDataIfNecessary( other );

			// then it's safe to set the size, data, and ref count object to the other shared data's
			m_Size      = other.m_Size;
			m_Data      = other.m_Data;
			m_RefCount  = other.m_RefCount;
			m_Allocator = other.m_Allocator;

			if ( m_RefCount )
			{
				(*m_RefCount)++;
			}
		}
		~SharedData()
		{
			if ( m_RefCount )
			{
				(*m_RefCount)--;

				if ( m_RefCount->getCount() == 0 )
				{
					if ( m_Data )
					{
						m_TotalBytesAllocated -= ( m_Size * sizeof(T) );
						if ( m_Allocator )
						{
							m_Allocator->free( m_Data );
						}
						else
						{
							delete[] m_Data;
						}
					}

					delete m_RefCount;
				}
			}
		}

		static SharedData MakeSharedData (unsigned int size, IAllocator* allocator = nullptr)
		{
			T* data = nullptr;
			if ( allocator )
			{
				data = reinterpret_cast<T*>( allocator->allocatePrimativeArray<uint8_t>(size * sizeof(T)) );
			}
			else
			{
				data = new T[size];
			}

			m_TotalBytesAllocated += ( size * sizeof(T) );

			return SharedData( size, data, allocator );
		}

		// This function does not delete the underlying data after losing all it's references, so should only be used for memory limited
		// applications where arrays need to be reused to save space
		static SharedData MakeSharedData (unsigned int size, T* data)
		{
			return SharedData( size, data, nullptr, false );
		}

		static SharedData MakeSharedDataFromRange (const SharedData& originalData, unsigned int startIndex, unsigned int endIndex)
		{
			if ( startIndex > endIndex )
			{
				unsigned int temp = startIndex;
				startIndex = endIndex;
				endIndex = temp;
			}
			else if ( startIndex == endIndex || startIndex >= originalData.getSize() || endIndex >= originalData.getSize() )
			{
				return SharedData::MakeSharedDataNull();
			}

			unsigned int newSize = ( endIndex - startIndex ) + 1;
			SharedData data = SharedData::MakeSharedData( newSize );

			for ( unsigned int index = 0; index < data.getSize(); index++ )
			{
				data[index] = originalData[startIndex + index];
			}

			return data;
		}

		static SharedData MakeSharedDataNull()
		{
			return SharedData();
		}

		static unsigned int GetTotalAllocatedBytes()
		{
			return m_TotalBytesAllocated;
		}

		T& get (unsigned int number = 0) const
		{
			return m_Data[number];
		}

		T* getPtr (unsigned int number = 0) const
		{
			return &m_Data[number];
		}

		unsigned int getSize() const { return m_Size; }

		unsigned int getSizeInBytes() const { return sizeof(T) * m_Size; }

		T& operator[] (int i) const
		{
			return this->get( i );
		}

		SharedData& operator= (const SharedData& other)
		{
			// if the underlying data is different, we need to first decrease the ref count of the previous data and delete if necessary
			this->decrementAndDeletePreviousUnderlyingDataIfNecessary (other);

			// then it's safe to set the size, data, and ref count object to the other shared data's
			m_Size      = other.m_Size;
			m_Data      = other.m_Data;
			m_RefCount  = other.m_RefCount;
			m_Allocator = other.m_Allocator;

			if ( m_RefCount )
			{
				(*m_RefCount)++;
			}

			return *this;
		}

	private:
		class Counter
		{
			public:
				Counter() : m_Count( 0 ) {}
				Counter (const Counter&) = delete;
				Counter& operator=(const Counter&) = delete;

				unsigned int getCount() { return m_Count; }

				void operator++()
				{
					m_Count++;
				}

				void operator++(int)
				{
					m_Count++;
				}

				void operator--()
				{
					m_Count--;
				}

				void operator--(int)
				{
					m_Count--;
				}

			private:
				unsigned int m_Count;
		};

		unsigned int 	m_Size = 0;
		T* 		m_Data = nullptr;
		Counter* 	m_RefCount = nullptr;
		IAllocator* 	m_Allocator = nullptr;

		SharedData (unsigned int size, T* data, IAllocator* allocator = nullptr, bool deleteUnderlyingDataIfNoRefs = true) :
			m_Size( size ),
			m_Data( data ),
			m_RefCount( (deleteUnderlyingDataIfNoRefs) ? new Counter() : nullptr ),
			m_Allocator( allocator )
		{
			if ( m_RefCount )
			{
				(*m_RefCount)++;
			}
		}

		SharedData() :
			m_Size( 0 ),
			m_Data( nullptr ),
			m_RefCount( new Counter() ),
			m_Allocator( nullptr )
		{
			(*m_RefCount)++;
		}

		void decrementAndDeletePreviousUnderlyingDataIfNecessary (const SharedData& other)
		{
			if ( m_RefCount )
			{
				if ( m_Data && m_Data != other.m_Data )
				{
					(*m_RefCount)--;

					if ( m_RefCount->getCount() == 0 )
					{
						if ( m_Data )
						{
							m_TotalBytesAllocated -= ( m_Size * sizeof(T) );
							if ( m_Allocator )
							{
								m_Allocator->free( m_Data );
							}
							else
							{
								delete[] m_Data;
							}
						}

						delete m_RefCount;
					}
				}
			}
		}
};

template <typename T>
unsigned int SharedData<T>::m_TotalBytesAllocated = 0;

#endif // SHAREDDATA_HPP
