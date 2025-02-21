#ifndef SDCARD_HPP
#define SDCARD_HPP

#include "LLPD.hpp"
#include "IStorageMedia.hpp"

class SDCard : public IStorageMedia
{
	public:
		SDCard (const SPI_NUM& spiNum, const GPIO_PORT& csPort, const GPIO_PIN& csPin);
		~SDCard();

		void writeToMedia (const SharedData<uint8_t>& data, const unsigned int address) override;
		SharedData<uint8_t> readFromMedia (const unsigned int sizeInBytes, const unsigned int address) override;
		void readFromMedia (const unsigned int address, const SharedData<uint8_t>& data) override;


		bool writeSingleBlock (const SharedData<uint8_t>& data, const unsigned int blockNum);
		bool writeMultipleBlocks (const SharedData<uint8_t>& data, const unsigned int startBlockNum);
		SharedData<uint8_t> readSingleBlock (const unsigned int blockNum);

		virtual bool needsInitialization() override { return false; }
		virtual void initialize() override; // this needs to be called before any writing or reading is done
		virtual void afterInitialize() override {}

	private:
		SPI_NUM 	m_SpiNum;
		GPIO_PORT 	m_CSPort; // chip select pin port
		GPIO_PIN 	m_CSPin;
		unsigned int 	m_BlockSize;
		bool 		m_UsingBlockAddressing; // true for block addressing, otherwise using byte addressing
		unsigned int 	m_ByteAddressingMultiplier; // 1 for block addressing, otherwise block size

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

		uint8_t sendCommand (uint8_t commandNum, uint8_t arg1, uint8_t arg2, uint8_t arg3, uint8_t arg4, uint8_t crc = 0x01,
					bool leaveCSLow =false);
		bool isVersion2Card();
		R1CommandResult interpretR1CommandResultByte (uint8_t commandResultByte);

		void setBlockSize (const unsigned int blockSize);
		unsigned int getBlockSize();

		SharedData<uint8_t> readOCR();
};

#endif // SDCARD_HPP
