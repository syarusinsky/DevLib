#include "SDCard.hpp"

SDCard::SDCard (const SPI_NUM& spiNum, const GPIO_PORT& csPort, const GPIO_PIN& csPin, bool hasMBR) :
	m_SpiNum( spiNum ),
	m_CSPort( csPort ),
	m_CSPin( csPin ),
	m_HasMBR( hasMBR )
{
}

SDCard::~SDCard()
{
}

void SDCard::writeToMedia (const SharedData<uint8_t>& data, const unsigned int address)
{
}

SharedData<uint8_t> SDCard::readFromMedia (const unsigned int sizeInBytes, const unsigned int address)
{
	return SharedData<uint8_t>::MakeSharedDataNull();
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

	// send software reset command (CMD0)
	uint8_t resultByte = this->sendCommand( 0, 0, 0, 0, 0, 0x95 );
	while ( resultByte == 0xFF )
	{
		resultByte = this->sendCommand( 0, 0, 0, 0, 0, 0x95 );
	}
	CommandResult result = this->interpretCommandResultByte( resultByte );

	// ensure the sd card is now in the idle state
	while ( ! result.IsInIdleState ) {} // TODO instead of a spinlock, initialize function should return a bool indicating success or not
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

SDCard::CommandResult SDCard::interpretCommandResultByte (uint8_t commandResultByte)
{
	CommandResult commandResult;

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
