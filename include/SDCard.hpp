#ifndef SDCARD_HPP
#define SDCARD_HPP

#include "LLPD.hpp"
#include "IStorageMedia.hpp"

class SDCard : public IStorageMedia
{
	public:
		SDCard (const SPI_NUM& spiNum, const GPIO_PORT& csPort, const GPIO_PIN& csPin, bool hasMBR = true);
		~SDCard();

		void writeToMedia (const SharedData<uint8_t>& data, const unsigned int address) override;
		SharedData<uint8_t> readFromMedia (const unsigned int sizeInBytes, const unsigned int address) override;

		bool writeSingleBlock (const SharedData<uint8_t>& data, const unsigned int blockNum); // returns false if write failed
		SharedData<uint8_t> readSingleBlock (const unsigned int blockNum);

		virtual bool needsInitialization() override { return false; }
		virtual void initialize() override; // this needs to be called before any writing or reading is done
		virtual void afterInitialize() override {}

		virtual bool hasMBR() override { return m_HasMBR; }

	private:
		SPI_NUM 	m_SpiNum;
		GPIO_PORT 	m_CSPort; // chip select pin port
		GPIO_PIN 	m_CSPin;
		bool 		m_HasMBR;
		unsigned int 	m_BlockSize;

		struct R1CommandResult
		{
			bool IsInIdleState 	= false;
			bool EraseReset 	= false;
			bool IllegalCommand 	= false;
			bool CommandCRCError 	= false;
			bool EraseSeqError 	= false;
			bool AddressError 	= false;
			bool ParameterError 	= false;
		};

		uint8_t sendCommand (uint8_t commandNum, uint8_t arg1, uint8_t arg2, uint8_t arg3, uint8_t arg4, uint8_t crc = 0xFF);
		bool isVersion2Card();
		R1CommandResult interpretR1CommandResultByte (uint8_t commandResultByte);

		void setBlockSize (const unsigned int blockSize);
		unsigned int getBlockSize();
};

#endif // SDCARD_HPP
