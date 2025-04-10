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

		Sram_23K256 (const SPI_NUM& spiNum, const GPIO_PORT& csPort, const GPIO_PIN& csPin);
		~Sram_23K256();

		// TODO currently this only works after several bytes are read and written to in byte mode, fix later!
		bool setSequentialMode (bool sequential); // if not set to sequential mode, the default byte mode is used
		bool getSequentialMode();

		void writeByte (uint16_t address, uint8_t data);
		uint8_t readByte (uint16_t address);

		void writeSequentialBytes (uint16_t startAddress, const SharedData<uint8_t>& data, bool useDma = false,
						GPIO_PORT* csPortForCallback = nullptr, GPIO_PIN* csPinForCallback = nullptr);
		SharedData<uint8_t> readSequentialBytes (uint16_t startAddress, unsigned int sizeInBytes);
		void readSequentialBytes (uint16_t startAddress, const SharedData<uint8_t>& data, bool useDma = false,
						GPIO_PORT* csPortForCallback = nullptr, GPIO_PIN* csPinForCallback = nullptr);

		// writeToMedia and readFromMedia do not use DMA, to use these functions with DMA you should use Sram_23K256_Manager
		void writeToMedia (const SharedData<uint8_t>& data, const unsigned int address) override;
		SharedData<uint8_t> readFromMedia (const unsigned int sizeInBytes, const unsigned int address) override;
		void readFromMedia (const unsigned int sizeInBytes, const SharedData<uint8_t>& data) override;

		virtual bool needsInitialization() override { return false; }
		virtual void initialize() override {}
		virtual void afterInitialize() override {}

	private:
		SPI_NUM   m_SpiNum;
		GPIO_PORT m_CSPort; // chip select pin port
		GPIO_PIN  m_CSPin;
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
		void readSequentialBytes (unsigned int startAddress, const SharedData<uint8_t>& data);

		void writeToMedia (const SharedData<uint8_t>& data, const unsigned int address) override;
		SharedData<uint8_t> readFromMedia (const unsigned int sizeInBytes, const unsigned int address) override;
		void readFromMedia (const unsigned int address, const SharedData<uint8_t>& data) override;

		virtual bool needsInitialization() override { return false; }
		virtual void initialize() override {}
		virtual void afterInitialize() override {}

		// sets the function pointers to the private static dma functions
		void setDmaMode (std::function<void()>* txComplete, std::function<void()>* rxComplete);
		void setDmaTransferCompleteCallback (std::function<void()> callback) { m_DmaTransferCompleteCallback = callback; }
		bool dmaTransferComplete();

	private:
		std::vector<Sram_23K256> 	m_Srams;
		bool 				m_DmaMode;
		bool 				m_DmaWriting; // if not writing, then reading
		GPIO_PORT 			m_CsPortForCallback; // on dma callbacks
		GPIO_PIN 			m_CsPinForCallback;
		std::vector<std::pair<std::pair<unsigned int, uint16_t>, SharedData<uint8_t>>> 	m_DmaQueue; // sram num, sram address, data
		std::function<void()> 		m_DmaTransferCompleteCallback;

		unsigned int clipStartAddress (unsigned int startAddress, unsigned int sizeInBytes, unsigned int sramNum);
		unsigned int clipEndAddress (unsigned int endAddress, unsigned int sizeInBytes, unsigned int sramNum);
		void writeSequentialBytesHelper (unsigned int startAddress, const SharedData<uint8_t>& data, unsigned int sramNum,
						unsigned int& dataIndex);
		void readSequentialBytesHelper (unsigned int startAddress, const SharedData<uint8_t>& data, unsigned int sramNum,
						unsigned int& dataIndex);

		void dmaTxCompleteCallback();
		void dmaRxCompleteCallback();
};

#endif // SRAM_23K256_HPP
