#ifndef ISTORAGEMEDIA_HPP
#define ISTORAGEMEDIA_HPP

/**************************************************************************
 * An IStorageMedia presents an interface for reading and writing to
 * a specific storage media, whether that be a hdd, eeprom, or any other
 * storage media.
**************************************************************************/

#include "SharedData.hpp"
#include <stdint.h>

class IStorageMedia
{
	public:
		virtual ~IStorageMedia() {}

		virtual void writeToMedia (const SharedData<uint8_t>& data, const unsigned int offsetInBytes) = 0;
		virtual SharedData<uint8_t> readFromMedia (const unsigned int sizeInBytes, const unsigned int offsetInBytes) = 0;
		virtual void readFromMedia (const unsigned int offsetInBytes, const SharedData<uint8_t>& data) = 0;

		virtual bool needsInitialization() = 0;
		virtual void initialize() = 0;
		virtual void afterInitialize() = 0;

		bool hasMBR()
		{
			SharedData<uint8_t> mbrSignature = this->readFromMedia( 2, 0x1FE );

			if ( mbrSignature[0] == 0x55 && mbrSignature[1] == 0xAA ) return true;

			return false;
		}
};

#endif // ISTORAGEMEDIA_HPP
