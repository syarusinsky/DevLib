#ifndef SHAREDDATA_HPP
#define SHAREDDATA_HPP

/*************************************************************************
 * The SharedData class is basically just a shared pointer. It's mostly
 * used for memory that needs to be allocated in IStorageMedia but
 * deleted outside of it.
*************************************************************************/

template <typename T>
class SharedData
{
	public:
		SharedData (const SharedData& other) :
			m_Size( other.m_Size ),
			m_Data( other.m_Data ),
			m_RefCount( other.m_RefCount )
		{
			(*m_RefCount)++;
		}
		~SharedData()
		{
			(*m_RefCount)--;

			if ( m_RefCount->getCount() == 0 )
			{
				if ( m_Data )
				{
					delete[] m_Data;
				}

				delete   m_RefCount;
			}
		}

		static SharedData MakeSharedData (unsigned int size)
		{
			return SharedData( size, new T[size] );
		}

		static SharedData MakeSharedDataNull()
		{
			return SharedData();
		}

		T& get (unsigned int number = 0)    const
		{
			if ( number < m_Size )
			{
				return m_Data[number];
			}

			return m_Data[0];
		}

		T* getPtr (unsigned int number = 0) const
		{
			if ( number < m_Size )
			{
				return &m_Data[number];
			}

			return nullptr;
		}

		unsigned int getSize() const { return m_Size; }

		unsigned int getSizeInBytes() const { return sizeof(T) * m_Size; }

		T& operator[] (int i)
		{
			return this->get( i );
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

		unsigned int 	m_Size;
		T* 		m_Data;
		Counter* 	m_RefCount;

		SharedData (unsigned int size, T* data) :
			m_Size( size ),
			m_Data( data ),
			m_RefCount( new Counter )
		{
		}

		SharedData() :
			m_Size( 0 ),
			m_Data( nullptr ),
			m_RefCount( new Counter )
		{
		}
};

#endif // SHAREDDATA_HPP
