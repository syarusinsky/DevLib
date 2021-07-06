#ifndef SRAM_23K256_HPP
#define SRAM_23K256_HPP

/**************************************************************************
 * An Sram_23K256 instance is an interface to a Microchip 23K256 256Kb
 * SPI SRAM. It provides functions to write individual
 * bytes as well as larger chunks of data.
**************************************************************************/

#include "LLPD.hpp"
#include "IStorageMedia.hpp"

#include <vector>

class Sram_23K256 : public IStorageMedia
{
	public:
		static const int SRAM_SIZE = 32768;

		Sram_23K256 (const SPI_NUM& spiNum, const GPIO_PORT& csPort, const GPIO_PIN& csPin, bool hasMBR = false);
		~Sram_23K256();

		// TODO currently this only works after several bytes are read and written to in byte mode, fix later!
		bool setSequentialMode (bool sequential); // if not set to sequential mode, the default byte mode is used
		bool getSequentialMode();

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

struct Sram_23K256_GPIO_Config
{
	GPIO_PORT 	m_CSPort;
	GPIO_PIN 	m_CSPin;

	Sram_23K256_GPIO_Config (const GPIO_PORT& port, const GPIO_PIN& pin) :
		m_CSPort( port ),
		m_CSPin( pin ) {}
};

// a simple class to manage more than one 23K256 on the same SPI bus
class Sram_23K256_Manager : public IStorageMedia
{
	public:
		Sram_23K256_Manager (const SPI_NUM& spiNum, const std::vector<Sram_23K256_GPIO_Config>& gpioConfigs);

		bool setSequentialMode (bool sequential);
		void writeByte (uint32_t adress, uint8_t data);
		uint8_t readByte (uint32_t address);
		void writeSequentialBytes (unsigned int startAddress, const SharedData<uint8_t>& data);
		SharedData<uint8_t> readSequentialBytes (unsigned int startAddress, unsigned int sizeInBytes);

		void writeToMedia (const SharedData<uint8_t>& data, const unsigned int address) override;
		SharedData<uint8_t> readFromMedia (const unsigned int sizeInBytes, const unsigned int address) override;

		bool needsInitialization() override { return false; }
		void initialize() override {}
		void afterInitialize() override {}

		bool hasMBR() override { return false; }

	private:
		std::vector<Sram_23K256> 	m_Srams;

		unsigned int clipStartAddress (unsigned int startAddress, unsigned int sizeInBytes, unsigned int sramNum);
		unsigned int clipEndAddress (unsigned int endAddress, unsigned int sizeInBytes, unsigned int sramNum);
		void writeSequentialBytesHelper (unsigned int startAddress, const SharedData<uint8_t>& data, unsigned int sramNum,
						unsigned int& dataIndex);
		void readSequentialBytesHelper (unsigned int startAddress, SharedData<uint8_t>& data, unsigned int sramNum,
						unsigned int& dataIndex);
};

#endif // SRAM_23K256_HPP
