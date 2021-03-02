#ifndef SRAM_23K256_HPP
#define SRAM_23K256_HPP

/**************************************************************************
 * An Sram_23K256 instance is an interface to a Microchip 23K256 256Kb
 * SPI SRAM. It provides functions to write individual
 * bytes as well as larger chunks of data.
**************************************************************************/

#include "LLPD.hpp"
#include "IStorageMedia.hpp"

class Sram_23K256 : public IStorageMedia
{
	public:
		static const int SRAM_SIZE = 65536;

		Sram_23K256 (const SPI_NUM& spiNum, const GPIO_PORT& csPort, const GPIO_PIN& csPin, bool hasMBR = false);
		~Sram_23K256();

		// TODO currently this only works after several bytes are read and written to in byte mode, fix later!
		bool setSequentialMode (bool sequential); // if not set to sequential mode, the default byte mode is used

		void writeByte (uint16_t address, uint8_t data);
		uint8_t readByte (uint16_t address);

		void writeSequentialBytes (uint16_t startAddress, const SharedData<uint8_t>& data);
		SharedData<uint8_t> readSequentialBytes (uint16_t startAddress, unsigned int sizeInBytes);

		void writeToMedia (const SharedData<uint8_t>& data, const unsigned int address) override;
		SharedData<uint8_t> readFromMedia (const unsigned int sizeInBytes, const unsigned int address) override;

		virtual bool needsInitialization() override { return false; }
		virtual void initialize() override {}
		virtual void afterInitialize() override {}

		virtual bool hasMBR() override { return m_HasMBR; }

	private:
		SPI_NUM   m_SpiNum;
		GPIO_PORT m_CSPort; // chip select pin port
		GPIO_PIN  m_CSPin;
		bool 	  m_HasMBR;
		bool      m_SequentialMode;

		uint8_t readStatusRegister();
		void writeStatusRegister (uint8_t regVal);
};

#endif // SRAM_23K256_HPP
