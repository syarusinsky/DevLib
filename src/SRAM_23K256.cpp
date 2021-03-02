#include "SRAM_23K256.hpp"

Sram_23K256::Sram_23K256 (const SPI_NUM& spiNum, const GPIO_PORT& csPort, const GPIO_PIN& csPin, bool hasMBR) :
	m_SpiNum( spiNum ),
	m_CSPort( csPort ),
	m_CSPin( csPin ),
	m_HasMBR( hasMBR ),
	m_SequentialMode( false )
{
}

Sram_23K256::~Sram_23K256()
{
}

uint8_t Sram_23K256::readStatusRegister()
{
	// pull cs low
	LLPD::gpio_output_set( m_CSPort, m_CSPin, false );

	// send read status register instruction
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0b00000101 );

	// recieve status register value
	uint8_t statusRegVal = LLPD::spi_master_send_and_recieve( m_SpiNum, 0b00000000 );

	// pull cs high
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );

	return statusRegVal;
}

void Sram_23K256::writeStatusRegister (uint8_t regVal)
{
	// pull cs low
	LLPD::gpio_output_set( m_CSPort, m_CSPin, false );

	// send write status register instruction
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0b00000001 );

	// send register value
	LLPD::spi_master_send_and_recieve( m_SpiNum, regVal );

	// pull cs high
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );
}

bool Sram_23K256::setSequentialMode (bool sequential)
{
	uint8_t statusRegVal = this->readStatusRegister();

	// clear mode bits, which sets the mode to byte mode
	statusRegVal &= ~( 0b11000000 );

	if ( sequential )
	{
		// set bits to set to sequential mode
		statusRegVal |= 0b01000000;

		m_SequentialMode = true;
	}
	else
	{
		m_SequentialMode = false;
	}

	this->writeStatusRegister( statusRegVal );

	// verify that status register was set correctly
	if ( this->readStatusRegister() != statusRegVal )
	{
		return false;
	}

	return true;
}

void Sram_23K256::writeByte (uint16_t address, uint8_t data)
{
	// pull cs low
	LLPD::gpio_output_set( m_CSPort, m_CSPin, false );

	// send write instruction
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0b00000010 );

	// send first half of address
	LLPD::spi_master_send_and_recieve( m_SpiNum, (address >> 8) );

	// send second half of address
	LLPD::spi_master_send_and_recieve( m_SpiNum, (address & 0b0000000011111111) );

	// send data
	LLPD::spi_master_send_and_recieve( m_SpiNum, data );

	// pull cs high
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );
}

uint8_t Sram_23K256::readByte (uint16_t address)
{
	// pull cs low
	LLPD::gpio_output_set( m_CSPort, m_CSPin, false );

	// send read instruction
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0b00000011 );

	// send first half of address
	LLPD::spi_master_send_and_recieve( m_SpiNum, (address >> 8) );

	// send second half of address
	LLPD::spi_master_send_and_recieve( m_SpiNum, (address & 0b0000000011111111) );

	// receive data
	uint8_t data = LLPD::spi_master_send_and_recieve( m_SpiNum, 0b00000000 );

	// pull cs high
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );

	return data;
}

void Sram_23K256::writeSequentialBytes (uint16_t startAddress, const SharedData<uint8_t>& data)
{
	// pull cs low
	LLPD::gpio_output_set( m_CSPort, m_CSPin, false );

	// send write instruction
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0b00000010 );

	// send first half of address
	LLPD::spi_master_send_and_recieve( m_SpiNum, (startAddress >> 8) );

	// send second half of address
	LLPD::spi_master_send_and_recieve( m_SpiNum, (startAddress & 0b0000000011111111) );

	for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
	{
		// send data
		LLPD::spi_master_send_and_recieve( m_SpiNum, data[byte] );
	}

	// pull cs high
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );
}

SharedData<uint8_t> Sram_23K256::readSequentialBytes (uint16_t startAddress, unsigned int sizeInBytes)
{
	// pull cs low
	LLPD::gpio_output_set( m_CSPort, m_CSPin, false );

	// send read instruction
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0b00000011 );

	// send first half of address
	LLPD::spi_master_send_and_recieve( m_SpiNum, (startAddress >> 8) );

	// send second half of address
	LLPD::spi_master_send_and_recieve( m_SpiNum, (startAddress & 0b0000000011111111) );

	SharedData<uint8_t> data = SharedData<uint8_t>::MakeSharedData( sizeInBytes );

	for ( unsigned int byte = 0; byte < sizeInBytes; byte++ )
	{
		// read data
		data[byte] = LLPD::spi_master_send_and_recieve( m_SpiNum, 0b00000000 );
	}

	// pull cs high
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );

	return data;
}

void Sram_23K256::writeToMedia (const SharedData<uint8_t>& data, const unsigned int address)
{
	if ( m_SequentialMode )
	{
		this->writeSequentialBytes( address, data );
	}
	else
	{
		uint8_t* dataPtr = data.getPtr();

		for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
		{
			this->writeByte( address + byte, dataPtr[byte] );
		}
	}
}

SharedData<uint8_t> Sram_23K256::readFromMedia (const unsigned int sizeInBytes, const unsigned int address)
{

	if ( m_SequentialMode )
	{
		return this->readSequentialBytes( address, sizeInBytes );
	}
	else
	{
		SharedData<uint8_t> data = SharedData<uint8_t>::MakeSharedData( sizeInBytes );
		uint8_t* dataPtr = data.getPtr();

		for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
		{
			dataPtr[byte] = this->readByte( address + byte );
		}

		return data;
	}
}
