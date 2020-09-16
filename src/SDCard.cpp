#include "SDCard.hpp"

#include <cmath>

#define VALID_R1_RESPONSE 0x00

SDCard::SDCard (const SPI_NUM& spiNum, const GPIO_PORT& csPort, const GPIO_PIN& csPin, bool hasMBR) :
	m_SpiNum( spiNum ),
	m_CSPort( csPort ),
	m_CSPin( csPin ),
	m_HasMBR( hasMBR ),
	m_BlockSize( 512 )
{
}

SDCard::~SDCard()
{
}

void SDCard::writeToMedia (const SharedData<uint8_t>& data, const unsigned int address)
{
	unsigned int dataSize = data.getSizeInBytes();

	unsigned int startBlock = address / m_BlockSize;
	unsigned int endBlock = ( address + dataSize ) / m_BlockSize;

	// get the number of bytes we'll need to keep from the beginning of the first block
	unsigned int bytesToSkip = address % m_BlockSize;

	unsigned int bytesSkipped = 0;
	unsigned int bytesWritten = 0;

	for ( unsigned int block = startBlock; block <= endBlock; block++ )
	{
		// get the original block from the sd card
		SharedData<uint8_t> blockToWrite = this->readFromMedia( m_BlockSize, block * m_BlockSize );

		// make modifications to the original block
		for ( unsigned int byte = 0; byte < m_BlockSize; byte++ )
		{
			if ( bytesWritten == dataSize )
			{
				break;
			}
			else if ( bytesSkipped > bytesToSkip )
			{
				blockToWrite[byte] = data[ bytesWritten ];
				bytesWritten++;
			}
			else
			{
				bytesSkipped++;
			}
		}

		// break block address into individual bytes
		uint8_t baByte1 = block & 0xFF;
		uint8_t baByte2 = ( block & 0xFF00     ) >> 8;
		uint8_t baByte3 = ( block & 0xFF0000   ) >> 16;
		uint8_t baByte4 = ( block & 0xFF000000 ) >> 24;

		// start single block write with CMD16
		uint8_t resultByte = this->sendCommand( 16, baByte4, baByte3, baByte2, baByte1 );
		while ( resultByte != VALID_R1_RESPONSE )
		{
			resultByte = this->sendCommand( 16, baByte4, baByte3, baByte2, baByte1 );
		}

		// we're about to write a block, so bring the cs pin low
		LLPD::gpio_output_set( m_CSPort, m_CSPin, false );

		// send two dummy bytes (at least one is required, but we'll be safe)
		LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );

		// send the start token (0xFE) for single block write
		LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFE );

		for ( unsigned int byte = 0; byte < m_BlockSize; byte++ )
		{
			LLPD::spi_master_send_and_recieve( m_SpiNum, blockToWrite[byte] );
		}

		// the full block is transferred, so we can bring cs pin high
		LLPD::gpio_output_set( m_CSPort, m_CSPin, true );

		// after data block write, we need to send CMD13
		this->sendCommand( 13, 0, 0, 0, 0 );
	}
}

SharedData<uint8_t> SDCard::readFromMedia (const unsigned int sizeInBytes, const unsigned int address)
{
	SharedData<uint8_t> dataToRead = SharedData<uint8_t>::MakeSharedData( sizeInBytes );

	unsigned int startBlock = address / m_BlockSize;
	unsigned int endBlock = ( address + sizeInBytes ) / m_BlockSize;

	// get the number of bytes we'll need to skip from the beginning of the first block
	unsigned int bytesToSkip = address % m_BlockSize;

	unsigned int bytesSkipped = 0;
	unsigned int bytesRead = 0;

	for ( unsigned int block = startBlock; block <= endBlock; block++ )
	{
		// break block address into individual bytes
		uint8_t baByte1 = block & 0xFF;
		uint8_t baByte2 = ( block & 0xFF00     ) >> 8;
		uint8_t baByte3 = ( block & 0xFF0000   ) >> 16;
		uint8_t baByte4 = ( block & 0xFF000000 ) >> 24;

		// start single block read with CMD17
		uint8_t resultByte = this->sendCommand( 17, baByte4, baByte3, baByte2, baByte1 );
		while ( resultByte != VALID_R1_RESPONSE )
		{
			resultByte = this->sendCommand( 17, baByte4, baByte3, baByte2, baByte1 );
		}

		// we're about to begin the block transfer, so bring cs pin low
		LLPD::gpio_output_set( m_CSPort, m_CSPin, false );

		// wait for transmission start byte (0xFE)
		uint8_t transmissionStartByte = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		while ( transmissionStartByte != 0xFE )
		{
			transmissionStartByte = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		}

		// make modifications to the original block
		for ( unsigned int byte = 0; byte < m_BlockSize; byte++ )
		{
			if ( bytesRead == sizeInBytes )
			{
				// these bytes aren't of interest to us
				LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
			}
			else if ( bytesSkipped > bytesToSkip )
			{
				dataToRead[byte] = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
				bytesRead++;
			}
			else
			{
				// these bytes aren't of interest to us
				LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
				bytesSkipped++;
			}
		}

		// send two dummy bytes
		LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );

		// bring cs pin high since the entire block is read
		LLPD::gpio_output_set( m_CSPort, m_CSPin, true );
	}

	return dataToRead;
}

void SDCard::setBlockSize (const unsigned int blockSize)
{
	m_BlockSize = blockSize;

	// break block size into individual bytes
	uint8_t bsByte1 = blockSize & 0xFF;
	uint8_t bsByte2 = ( blockSize & 0xFF00     ) >> 8;
	uint8_t bsByte3 = ( blockSize & 0xFF0000   ) >> 16;
	uint8_t bsByte4 = ( blockSize & 0xFF000000 ) >> 24;

	uint8_t resultByte = this->sendCommand( 16, bsByte4, bsByte3, bsByte2, bsByte1 );
	while ( resultByte != VALID_R1_RESPONSE ) // TODO probably want to do some sort of timeout here instead of a spinlock forever
	{
		resultByte = this->sendCommand( 16, bsByte4, bsByte3, bsByte2, bsByte1 );
	}
}

unsigned int SDCard::getBlockSize()
{
	return m_BlockSize;
}

void SDCard::test()
{
	// initialize card

	// first send 80 dummy bytes to get the clock juices flowing
	LLPD::gpio_output_set( m_CSPort, m_CSPin, false );
	for ( unsigned int dummyByteNum = 0; dummyByteNum < 80; dummyByteNum++ )
	{
		LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
	}
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );

	// these will be used with each command response
	uint8_t resultByte = 0xFF;
	R1CommandResult result;

	// send software reset command (CMD0)
	resultByte = this->sendCommand( 0, 0, 0, 0, 0, 0x95 );
	while ( resultByte == 0xFF )
	{
		resultByte = this->sendCommand( 0, 0, 0, 0, 0, 0x95 );
	}
	result = this->interpretR1CommandResultByte( resultByte );

	// ensure the sd card is now in the idle state
	while ( ! result.IsInIdleState ) {} // TODO instead of a spinlock, initialize function should return a bool indicating success or not

	// send (CMD8) to find out which version the sd card is
	uint8_t cardVersion = 0;
	resultByte = this->sendCommand( 8, 0, 0, 0x01, 0xAA, 0x87 );
	if ( resultByte == 0x01 )
	{
		// 4 additional bytes may be sent, so let's just send dummy values -_(o.o)_-
		LLPD::gpio_output_set( m_CSPort, m_CSPin, false );
		LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		LLPD::gpio_output_set( m_CSPort, m_CSPin, true );

		cardVersion = 2;
	}
	else if ( resultByte == 0x05 )
	{
		cardVersion = 1;
	}

	while ( cardVersion == 0 ) {} // TODO instead of a spinlock, initialize function should return a bool indicating success or not

	// TODO CMD58? Eventually we want should verify the card is working at the correct voltage

	// ensure the card is out of the idle state (CMD1)
	resultByte = this->sendCommand( 1, 0, 0, 0, 0 );
	while ( resultByte != VALID_R1_RESPONSE )
	{
		resultByte = this->sendCommand( 1, 0, 0, 0, 0 );
	}

	// set initial block size to 512
	this->setBlockSize( 512 );
}

uint8_t SDCard::sendCommand (uint8_t commandNum, uint8_t arg1, uint8_t arg2, uint8_t arg3, uint8_t arg4, uint8_t crc)
{
	// pull cs low
	LLPD::gpio_output_set( m_CSPort, m_CSPin, false );

	LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF ); // good practice to send an extra dummy byte

	uint8_t commandByte = 64 + commandNum; // command values are always 64 plus the command num
	LLPD::spi_master_send_and_recieve( m_SpiNum, commandByte );

	LLPD::spi_master_send_and_recieve( m_SpiNum, arg1 );

	LLPD::spi_master_send_and_recieve( m_SpiNum, arg2 );

	LLPD::spi_master_send_and_recieve( m_SpiNum, arg3 );

	LLPD::spi_master_send_and_recieve( m_SpiNum, arg4 );

	LLPD::spi_master_send_and_recieve( m_SpiNum, crc ); // TODO CRC byte, should we do something with this?

	// keep recieving bytes until the response flag is set
	uint8_t response = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
	uint8_t responseTimeout = 0;
	while ( response & 0b10000000 && responseTimeout < 255 )
	{
		response = LLPD::spi_master_send_and_recieve( m_SpiNum, 0xFF );
		responseTimeout++;
	}

	// pull cs high
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );

	return response;
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
