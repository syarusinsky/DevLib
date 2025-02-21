#include "FakeStorageDevice.hpp"

FakeStorageDevice::FakeStorageDevice (const unsigned int deviceSizeInBytes) :
	m_SizeInBytes( deviceSizeInBytes ),
	m_DataArray( new uint8_t[deviceSizeInBytes] )
{
}

FakeStorageDevice::~FakeStorageDevice()
{
	delete[] m_DataArray;
}

void FakeStorageDevice::writeByte (uint16_t address, uint8_t data)
{
	m_DataArray[address] = data;
}

uint8_t FakeStorageDevice::readByte (uint16_t address)
{
	return m_DataArray[address];
}

void FakeStorageDevice::writeToMedia (const SharedData<uint8_t>& data, const unsigned int offsetInBytes)
{
	if ( data.getSizeInBytes() + offsetInBytes <= m_SizeInBytes ) // if the data fits in this media
	{
		for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte ++ )
		{
			m_DataArray[offsetInBytes + byte] = data[byte];
		}
	}
}

SharedData<uint8_t> FakeStorageDevice::readFromMedia (const unsigned int sizeInBytes, const unsigned int offsetInBytes)
{
	SharedData<uint8_t> data = SharedData<uint8_t>::MakeSharedData( sizeInBytes );

	for ( unsigned int byte = 0; byte < sizeInBytes; byte++ )
	{
		data[byte] = m_DataArray[offsetInBytes + byte];
	}

	return data;
}

void FakeStorageDevice::readFromMedia (const unsigned int offsetInBytes, const SharedData<uint8_t>& data)
{
	for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
	{
		data[byte] = m_DataArray[offsetInBytes + byte];
	}
}
