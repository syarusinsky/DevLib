#include "SRAM_23K256.hpp"

Sram_23K256::Sram_23K256 (const SPI_NUM& spiNum, const GPIO_PORT& csPort, const GPIO_PIN& csPin) :
	m_SpiNum( spiNum ),
	m_CSPort( csPort ),
	m_CSPin( csPin )
{
}

Sram_23K256::~Sram_23K256()
{
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
