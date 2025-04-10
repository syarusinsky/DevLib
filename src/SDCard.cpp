#include "SDCard.hpp"

#include <cmath>

#define VALID_R1_RESPONSE 0x00

SDCard::SDCard (const SPI_NUM& spiNum, const GPIO_PORT& csPort, const GPIO_PIN& csPin) :
	m_SpiNum( spiNum ),
	m_CSPort( csPort ),
	m_CSPin( csPin ),
	m_BlockSize( 512 ),
	m_UsingBlockAddressing( false ),
	m_ByteAddressingMultiplier( 1 )
{
}

SDCard::~SDCard()
{
}

void SDCard::writeToMedia (const SharedData<uint8_t>& data, const unsigned int address)
{
	unsigned int dataSize = data.getSizeInBytes();

	unsigned int startBlock = address / m_BlockSize;
	unsigned int endBlock = ( address + dataSize - 1 ) / m_BlockSize;

	// no need to read a block if we're writing an entire single block, note: this only speeds things up for writing single blocks
	if ( (dataSize % m_BlockSize == 0) && (startBlock == endBlock) )
	{
		this->writeSingleBlock( data, startBlock );

		return;
	}

	// get the number of bytes we'll need to keep from the beginning of the first block
	unsigned int bytesToSkip = address % m_BlockSize;

	unsigned int bytesSkipped = 0;
	unsigned int bytesWritten = 0;

	for ( unsigned int block = startBlock; block <= endBlock; block++ )
	{
		// get the original block from the sd card
		SharedData<uint8_t> blockToWrite = this->readSingleBlock( block );

		// make modifications to the original block
		for ( unsigned int byte = 0; byte < m_BlockSize; byte++ )
		{
			if ( bytesWritten == dataSize )
			{
				break;
			}
			else if ( bytesSkipped == bytesToSkip )
			{
				blockToWrite[byte] = data[bytesWritten];
				bytesWritten++;
			}
			else
			{
				bytesSkipped++;
			}
		}

		this->writeSingleBlock( blockToWrite, block );
	}
}

SharedData<uint8_t> SDCard::readFromMedia (const unsigned int sizeInBytes, const unsigned int address)
{
	SharedData<uint8_t> dataToRead = SharedData<uint8_t>::MakeSharedData( sizeInBytes );

	unsigned int startBlock = address / m_BlockSize;
	unsigned int endBlock = ( address + sizeInBytes - 1 ) / m_BlockSize;

	// get the number of bytes we'll need to skip from the beginning of the first block
	unsigned int bytesToSkip = address % m_BlockSize;

	unsigned int bytesSkipped = 0;
	unsigned int bytesRead = 0;

	for ( unsigned int block = startBlock; block <= endBlock; block++ )
	{
		SharedData<uint8_t> blockData = this->readSingleBlock( block );

		// make modifications to the original block
		for ( unsigned int byte = 0; byte < m_BlockSize; byte++ )
		{
			if ( bytesRead == sizeInBytes )
			{
				// we've finished reading
				break;
			}
			else if ( bytesSkipped == bytesToSkip )
			{
				dataToRead[bytesRead] = blockData[byte];
				bytesRead++;
			}
			else
			{
				// these bytes aren't of interest to us
				bytesSkipped++;
			}
		}
	}

	return dataToRead;
}

void SDCard::readFromMedia (const unsigned int address, const SharedData<uint8_t>& data)
{
	uint8_t* dataToReadPtr = data.getPtr();
	unsigned int sizeInBytes = data.getSizeInBytes();

	unsigned int startBlock = address / m_BlockSize;
	unsigned int endBlock = ( address + sizeInBytes - 1 ) / m_BlockSize;

	// get the number of bytes we'll need to skip from the beginning of the first block
	unsigned int bytesToSkip = address % m_BlockSize;

	unsigned int bytesSkipped = 0;
	unsigned int bytesRead = 0;

	for ( unsigned int block = startBlock; block <= endBlock; block++ )
	{
		SharedData<uint8_t> blockData = this->readSingleBlock( block );

		// make modifications to the original block
		for ( unsigned int byte = 0; byte < m_BlockSize; byte++ )
		{
			if ( bytesRead == sizeInBytes )
			{
				// we've finished reading
				break;
			}
			else if ( bytesSkipped == bytesToSkip )
			{
				dataToReadPtr[bytesRead] = blockData[byte];
				bytesRead++;
			}
			else
			{
				// these bytes aren't of interest to us
				bytesSkipped++;
			}
		}
	}
}

void SDCard::setBlockSize (const unsigned int blockSize)
{
	m_BlockSize = blockSize;

	m_ByteAddressingMultiplier = ( m_UsingBlockAddressing ) ? 1 : m_BlockSize;

	// break block size into individual bytes
	uint8_t bsByte1 = blockSize & 0xFF;
	uint8_t bsByte2 = ( blockSize & 0xFF00     ) >> 8;
	uint8_t bsByte3 = ( blockSize & 0xFF0000   ) >> 16;
	uint8_t bsByte4 = ( blockSize & 0xFF000000 ) >> 24;

	uint8_t resultByte = this->sendCommand( 16, bsByte1, bsByte2, bsByte3, bsByte4 );
	while ( resultByte != VALID_R1_RESPONSE )
	{
		LLPD::tim6_delay( 10000 );
		resultByte = this->sendCommand( 16, bsByte1, bsByte2, bsByte3, bsByte4 );
	}
}

unsigned int SDCard::getBlockSize()
{
	return m_BlockSize;
}

void SDCard::initialize()
{
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );

	// wait for 1 second (only 1ms required, but we're being extra safe)
	LLPD::tim6_delay( 1000000 );

	// first send 80 dummy bytes to get the clock juices flowing
	for ( unsigned int dummyByteNum = 0; dummyByteNum < 80; dummyByteNum++ )
	{
		LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
	}

	// wait for 1.5 ms
	LLPD::tim6_delay( 1500 );

	// these will be used with each command response
	uint8_t resultByte = 0xFF;
	R1CommandResult result;

	// send software reset command (CMD0)
	resultByte = this->sendCommand( 0, 0, 0, 0, 0, 0x94 );
	result = this->interpretR1CommandResultByte( resultByte );
	while ( result.IllegalCommand || ! result.IsInIdleState )
	{
		resultByte = this->sendCommand( 0, 0, 0, 0, 0, 0x94 );
		result = this->interpretR1CommandResultByte( resultByte );
		LLPD::tim6_delay( 10000 );
	}

	LLPD::tim6_delay( 10000 );

	// send (CMD8) to find out which version the sd card is
	bool cardVersion2 = this->isVersion2Card();

	LLPD::tim6_delay( 10000 );

	if ( cardVersion2 )
	{
		// ensure the card is out of the idle state (ACMD41)
		unsigned int attempts = 100;
		resultByte = this->sendCommand( 55, 0, 0, 0, 0, 0x65 );
		resultByte = this->sendCommand( 41, 0x40, 0, 0, 0, 0x77 );
		while ( resultByte != VALID_R1_RESPONSE && attempts > 0 )
		{
			resultByte = this->sendCommand( 55, 0, 0, 0, 0, 0x65 );
			resultByte = this->sendCommand( 41, 0x40, 0, 0, 0, 0x77 );
			attempts--;
			LLPD::tim6_delay( 10000 );
		}

		// try (CMD1) if (ACMD41) fails
		if ( attempts == 0 )
		{
			// ensure the card is out of the idle state (CMD1)
			resultByte = this->sendCommand( 1, 0, 0, 0, 0, 0xF9 );
			while ( resultByte != VALID_R1_RESPONSE )
			{
				resultByte = this->sendCommand( 1, 0, 0, 0, 0, 0xF9 );
				LLPD::tim6_delay( 10000 );
			}
		}
	}
	else
	{
		// ensure the card is out of the idle state (CMD1)
		resultByte = this->sendCommand( 1, 0, 0, 0, 0, 0xF9 );
		while ( resultByte != VALID_R1_RESPONSE )
		{
			resultByte = this->sendCommand( 1, 0, 0, 0, 0, 0xF9 );
			LLPD::tim6_delay( 10000 );
		}
	}

	// CMD58 so that we can determine if we have high-capacity card and need byte addressing, or block addressing
	SharedData<uint8_t> ocr = this->readOCR();
	m_UsingBlockAddressing = ocr[0] & 0b01000000;

	// set initial block size to 512
	this->setBlockSize( 512 );
}

uint8_t SDCard::sendCommand (uint8_t commandNum, uint8_t arg1, uint8_t arg2, uint8_t arg3, uint8_t arg4, uint8_t crc, bool leaveCSLow)
{
	// pull cs low
	LLPD::gpio_output_set( m_CSPort, m_CSPin, false );

	// dummy byte
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );

	LLPD::spi_master_send_and_recieve( m_SpiNum, (commandNum | 0x40) );

	LLPD::spi_master_send_and_recieve( m_SpiNum, arg4 );

	LLPD::spi_master_send_and_recieve( m_SpiNum, arg3 );

	LLPD::spi_master_send_and_recieve( m_SpiNum, arg2 );

	LLPD::spi_master_send_and_recieve( m_SpiNum, arg1 );

	LLPD::spi_master_send_and_recieve( m_SpiNum, (crc | 0x01) ); // TODO CRC byte, should we do something with this?

	// keep recieving bytes until the response flag is set
	uint8_t response = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
	uint8_t responseTimeout = 0;
	while ( response == 0xFF && responseTimeout < 255 )
	{
		response = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		responseTimeout++;
	}

	// pull cs high (or leave low)
	LLPD::gpio_output_set( m_CSPort, m_CSPin, ! leaveCSLow );

	return response;
}

bool SDCard::isVersion2Card()
{
	bool retVal = false;
	unsigned int timeout = 0;

Retry:
	// pull cs low
	LLPD::gpio_output_set( m_CSPort, m_CSPin, false );

	LLPD::spi_master_send_and_recieve( m_SpiNum, (8 | 0x40) ); // CMD8

	LLPD::spi_master_send_and_recieve( m_SpiNum, 0 );

	LLPD::spi_master_send_and_recieve( m_SpiNum, 0 );

	LLPD::spi_master_send_and_recieve( m_SpiNum, 0x01 );

	LLPD::spi_master_send_and_recieve( m_SpiNum, 0xAA );

	LLPD::spi_master_send_and_recieve( m_SpiNum, (0x86 | 0x01) );

	// keep recieving bytes until the R1 response is received
	uint8_t response = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
	uint8_t responseTimeout = 0;
	while ( response == 0xFF && responseTimeout < 255 )
	{
		response = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		responseTimeout++;
	}

	if ( response == 0xFF || response == 0x05 )
	{
		// pull cs high
		LLPD::gpio_output_set( m_CSPort, m_CSPin, true );

		// dummy byte
		LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );

		return false; // version 1 card or unknown
	}
	else if ( response == 0x01 )
	{
		// probably version 2 card
		retVal = true;

		// check for echo
		response = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		responseTimeout = 0;
		while ( response == 0xFF && responseTimeout < 255 )
		{
			response = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
			responseTimeout++;
		}

		if ( response != 0 ) retVal = false;
		response = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		if ( response != 0 ) retVal = false;
		response = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		if ( response != 0x01 ) retVal = false;
		response = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		if ( response != 0xAA ) retVal = false;

		// dummy byte
		LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );

		// pull cs high
		LLPD::gpio_output_set( m_CSPort, m_CSPin, true );

		timeout++;

		// wait for 1.5 ms
		LLPD::tim6_delay( 1500 );

		if ( retVal == false && timeout < 255 ) goto Retry;
	}

	// pull cs high
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );

	return retVal;
}

SDCard::R1CommandResult SDCard::interpretR1CommandResultByte (uint8_t commandResultByte)
{
	R1CommandResult commandResult;

	if ( (commandResultByte & 0b00000001) )
	{
		commandResult.IsInIdleState = true;
	}

	if ( (commandResultByte & 0b00000010) )
	{
		commandResult.EraseReset = true;
	}

	if ( (commandResultByte & 0b00000100) )
	{
		commandResult.IllegalCommand = true;
	}

	if ( (commandResultByte & 0b00001000) )
	{
		commandResult.CommandCRCError = true;
	}

	if ( (commandResultByte & 0b00010000) )
	{
		commandResult.EraseSeqError = true;
	}

	if ( (commandResultByte & 0b00100000) )
	{
		commandResult.AddressError = true;
	}

	if ( (commandResultByte & 0b01000000) )
	{
		commandResult.ParameterError = true;
	}

	return commandResult;
}

bool SDCard::writeSingleBlock (const SharedData<uint8_t>& data, const unsigned int blockNum)
{
	// if byte addressing, we need to multiply by the block size
	const unsigned int address = blockNum * m_ByteAddressingMultiplier;

	// unsure the data is block sized
	if ( data.getSize() != m_BlockSize ) return false;

	// break block address into individual bytes
	uint8_t baByte1 = address & 0xFF;
	uint8_t baByte2 = ( address & 0xFF00     ) >> 8;
	uint8_t baByte3 = ( address & 0xFF0000   ) >> 16;
	uint8_t baByte4 = ( address & 0xFF000000 ) >> 24;

	// start single block write with CMD24
	uint8_t resultByte = this->sendCommand( 24, baByte1, baByte2, baByte3, baByte4, 0xFF, true );
	while ( resultByte != VALID_R1_RESPONSE )
	{
		resultByte = this->sendCommand( 24, baByte1, baByte2, baByte3, baByte4, 0xFF, true );
	}

	// send two dummy bytes (at least one is required, but we'll be safe)
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );

	// send the start token (0xFE) for single block write
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFE );

	for ( unsigned int byte = 0; byte < m_BlockSize; byte++ )
	{
		LLPD::spi_master_send_and_recieve( m_SpiNum, data[byte] );
	}

	// wait for response
	resultByte = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
	while ( (resultByte & 0x1F) != 0x05 )
	{
		resultByte = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
	}

	// wait until no longer busy (finished writing)
	resultByte = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
	while ( resultByte != 0xFF )
	{
		resultByte = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
	}

	// send two dummy bytes for safety
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );

	// the full block is transferred, so we can bring cs pin high
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );

	return true;
}

SharedData<uint8_t> SDCard::readSingleBlock (unsigned int blockNum)
{
	// if byte addressing, we need to multiply by the block size
	const unsigned int address = blockNum * m_ByteAddressingMultiplier;

	SharedData<uint8_t> readBlockData = SharedData<uint8_t>::MakeSharedData( m_BlockSize );

	// break block address into individual bytes
	uint8_t baByte1 = address & 0xFF;
	uint8_t baByte2 = ( address & 0xFF00     ) >> 8;
	uint8_t baByte3 = ( address & 0xFF0000   ) >> 16;
	uint8_t baByte4 = ( address & 0xFF000000 ) >> 24;

	// start single block read with CMD17
	uint8_t resultByte = this->sendCommand( 17, baByte1, baByte2, baByte3, baByte4, 0xFF, true );
	while ( resultByte != VALID_R1_RESPONSE )
	{
		resultByte = this->sendCommand( 17, baByte1, baByte2, baByte3, baByte4, 0xFF, true );
	}

	// wait for transmission start byte (0xFE)
	uint8_t transmissionStartByte = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
	while ( transmissionStartByte != 0xFE )
	{
		transmissionStartByte = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
	}

	// read data into buffer
	for ( unsigned int byte = 0; byte < m_BlockSize; byte++ )
	{
		readBlockData[byte] = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
	}

	// send two dummy bytes (actually to read CRC, but we don't care)
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );

	// bring cs pin high since the entire block is read
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );

	return readBlockData;
}

bool SDCard::writeMultipleBlocks (const SharedData<uint8_t>& data, const unsigned int startBlockNum)
{
	// if byte addressing, we need to multiply by the block size
	const unsigned int address = startBlockNum * m_ByteAddressingMultiplier;

	// unsure the data is block sized
	if ( data.getSize() % m_BlockSize != 0 ) return false;

	const unsigned int numBlocksToWrite = data.getSize() / m_BlockSize;

	// break block address into individual bytes
	uint8_t baByte1 = address & 0xFF;
	uint8_t baByte2 = ( address & 0xFF00     ) >> 8;
	uint8_t baByte3 = ( address & 0xFF0000   ) >> 16;
	uint8_t baByte4 = ( address & 0xFF000000 ) >> 24;

	// start multiple block write with CMD25
	uint8_t resultByte = this->sendCommand( 25, baByte1, baByte2, baByte3, baByte4, 0xFF, true );
	while ( resultByte != VALID_R1_RESPONSE )
	{
		resultByte = this->sendCommand( 25, baByte1, baByte2, baByte3, baByte4, 0xFF, true );
	}

	for ( unsigned int block = 0; block < numBlocksToWrite; block++ )
	{
		// send two dummy bytes (at least one is required, but we'll be safe)
		LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );

		// send the start token (0xFC) for multiple block write
		LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFC );

		// write blocks
		for ( unsigned int byte = 0; byte < m_BlockSize; byte++ )
		{
			LLPD::spi_master_send_and_recieve( m_SpiNum, data[(m_BlockSize * block) + byte] );
		}

		// two dummy bytes for crc
		LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );

		// wait for response
		resultByte = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		while ( (resultByte & 0x1F) != 0x05 )
		{
			resultByte = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		}

		// wait until no longer busy (finished writing)
		resultByte = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		while ( resultByte != 0xFF )
		{
			resultByte = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		}
	}

	// send stop transmission token (0xFD)
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFD );

	// dummy byte necessary after stop transmission token
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );

	// wait until no longer busy (finished writing)
	resultByte = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
	while ( resultByte != 0xFF )
	{
		resultByte = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
	}

	// send two dummy bytes for safety
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );

	// the full block is transferred, so we can bring cs pin high
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );

	return true;
}

SharedData<uint8_t> SDCard::readOCR()
{
	constexpr unsigned int ocrSize = sizeof( uint32_t );

	SharedData<uint8_t> ocrContents = SharedData<uint8_t>::MakeSharedData( ocrSize );

	// start ocr read with CMD58
	uint8_t resultByte = this->sendCommand( 58, 0, 0, 0, 0, 0x00, true );
	while ( resultByte != VALID_R1_RESPONSE )
	{
		resultByte = this->sendCommand( 58, 0, 0, 0, 0, 0x00, true );
	}

	// read data into buffer
	for ( unsigned int byte = 0; byte < ocrSize; byte++ )
	{
		ocrContents[byte] = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
	}

	// send two dummy bytes
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );

	// bring cs pin high since the entire ocr is read
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );

	return ocrContents;
}
