#ifndef FAKESTORAGEDEVICE_HPP
#define FAKESTORAGEDEVICE_HPP

/**************************************************************************
 * A FakeStorageDevice simulates a physical storage device such as a
 * eeprom, sram, ect.
**************************************************************************/

#include "IStorageMedia.hpp"

class FakeStorageDevice : public IStorageMedia
{
	public:
		FakeStorageDevice (const unsigned int deviceSizeInBytes);
		~FakeStorageDevice() override;

		void writeByte (uint16_t address, uint8_t data);
		uint8_t readByte (uint16_t address);

		void writeToMedia (const SharedData<uint8_t>& data, const unsigned int offsetInBytes) override;
		SharedData<uint8_t> readFromMedia (const unsigned int sizeInBytes, const unsigned int offsetInBytes) override;

		bool needsInitialization() override { return false; }
		void initialize() override {}
		void afterInitialize() override {}

		bool hasMBR() override { return false; }

	private:
		unsigned int 	m_SizeInBytes;
		uint8_t* 	m_DataArray;
};

#endif // FAKESTORAGEDEVICE_HPP
