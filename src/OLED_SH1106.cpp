#include "OLED_SH1106.hpp"

Oled_SH1106::Oled_SH1106 (const SPI_NUM& spiNum, const GPIO_PORT& csPort, const GPIO_PIN& csPin, const GPIO_PORT& dcPort,
				const GPIO_PIN& dcPin, const GPIO_PORT& rstPort, const GPIO_PIN& rstPin) :
	m_SpiNum( spiNum ),
	m_CSPort( csPort ),
	m_CSPin( csPin ),
	m_DCPort( dcPort ),
	m_DCPin( dcPin ),
	m_RstPort( rstPort ),
	m_RstPin( rstPin )
{
}

Oled_SH1106::~Oled_SH1106()
{
}

void Oled_SH1106::begin()
{
	this->reset();

	// the display needs to be off for certain commands to be issued
	this->sendCommand( SH1106_SETDISPLAYOFF );

	// set the clock prescaler
	this->sendCommand( SH1106_SETDISPLAYCLOCKDIV );
	this->sendCommand( 0x80 ); // TODO this may need to be tweaked

	// set multiplex ration mode
	this->sendCommand( SH1106_SETMULTIPLEX );
	this->sendCommand( 0x3F ); // TODO this may need to be tweaked

	// set diplay offset
	this->sendCommand( SH1106_SETDISPLAYOFFSET );
	this->sendCommand( 0x00 ); // shouldn't need an offset

	// set chargepump voltage
	this->sendCommand( SH1106_SETCHARGEPUMP6_4V );

	// set the segment direction
	this->sendCommand( SH1106_SETSEGREMAPRIGHT );

	// set the com scan direction
	this->sendCommand( SH1106_SETCOMSCANDEC );

	// set the common pads hardware configuration to alternative (0x02 would be sequential)
	this->sendCommand( SH1106_SETCOMPINS );
	this->sendCommand( 0x12 );

	// set the contrast
	this->sendCommand( SH1106_SETCONTRAST );
	this->sendCommand( 0xCF ); // TODO this may need to be tweaked

	// set the charge/discharge periods for the charge pump
	this->sendCommand( SH1106_SETPRECHARGE );
	this->sendCommand( 0xF1 ); // TODO this may need to be tweaked

	// set the common pad output voltage level at the deselect stage
	this->sendCommand( SH1106_SETVCOMDETECT );
	this->sendCommand( 0x40 );

	// set the display layout to normal
	this->sendCommand( SH1106_SETNORMALDISPLAY );

	// set the RAM start line (this can be used for scrolling, but for now we'll leave it here)
	this->sendCommand( SH1106_SETSTARTLINE );

	// turn the display back on
	this->sendCommand( SH1106_SETDISPLAYON );
}

void Oled_SH1106::displayFullRowMajor (uint8_t* buffer)
{
	for ( unsigned int page = 0; page < SH1106_NUMPAGES; page++ )
	{
		this->sendCommand( SH1106_SETPAGEADDRESS | page );
		this->sendCommand( SH1106_SETLOWCOLUMN | 0x2 ); // since we aren't using scrolling,
								// the first and last 2 columns don't matter
		this->sendCommand( SH1106_SETHIGHCOLUMN | 0x0 );
		this->sendCommand( SH1106_SETSTARTLINE | 0x0 );

		unsigned int pageOffset = page * SH1106_LCDWIDTH;

		for ( unsigned int column = 0; column < SH1106_LCDWIDTH; column++ )
		{
			uint8_t colAdj = 7 - ( column % 8 );
			unsigned int rowOffset = SH1106_LCDWIDTH / 8;

			uint8_t newByte = 0x0;
			newByte |= (( buffer[(column / 8) + pageOffset + (rowOffset * 0)] & (1 << colAdj) ) >> colAdj << 0);
			newByte |= (( buffer[(column / 8) + pageOffset + (rowOffset * 1)] & (1 << colAdj) ) >> colAdj << 1);
			newByte |= (( buffer[(column / 8) + pageOffset + (rowOffset * 2)] & (1 << colAdj) ) >> colAdj << 2);
			newByte |= (( buffer[(column / 8) + pageOffset + (rowOffset * 3)] & (1 << colAdj) ) >> colAdj << 3);
			newByte |= (( buffer[(column / 8) + pageOffset + (rowOffset * 4)] & (1 << colAdj) ) >> colAdj << 4);
			newByte |= (( buffer[(column / 8) + pageOffset + (rowOffset * 5)] & (1 << colAdj) ) >> colAdj << 5);
			newByte |= (( buffer[(column / 8) + pageOffset + (rowOffset * 6)] & (1 << colAdj) ) >> colAdj << 6);
			newByte |= (( buffer[(column / 8) + pageOffset + (rowOffset * 7)] & (1 << colAdj) ) >> colAdj << 7);

			this->sendData( newByte );
		}
	}
}

void Oled_SH1106::displayPartialRowMajor (uint8_t* buffer, uint8_t startRow, uint8_t startCol, uint8_t endRow, uint8_t endCol)
{
	// ensure the partial is within the bounds
	if ( startRow < SH1106_LCDHEIGHT && startCol < SH1106_LCDWIDTH && endRow < SH1106_LCDHEIGHT && endCol < SH1106_LCDWIDTH )
	{
		// ensure we have the right order
		if ( startRow > endRow )
		{
			uint8_t temp = startRow;
			startRow = endRow;
			endRow = temp;
		}

		if ( startCol > endCol )
		{
			uint8_t temp = startCol;
			startCol = endCol;
			endCol = temp;
		}

		uint8_t startPage = startRow / 8;
		uint8_t endPage = ( endRow / 8 ) + 1;

		for ( unsigned int page = startPage; page < endPage; page++ )
		{
			uint8_t colAddrLow  = (startCol + 2) & 0x0F; // plus 2 since the first and last 2 columns don't matter
			uint8_t colAddrHigh = (startCol + 2) >> 4;

			this->sendCommand( SH1106_SETPAGEADDRESS | page );
			this->sendCommand( SH1106_SETLOWCOLUMN | colAddrLow );
			this->sendCommand( SH1106_SETHIGHCOLUMN | colAddrHigh );
			this->sendCommand( SH1106_SETSTARTLINE | 0x0 );

			unsigned int pageOffset = page * SH1106_LCDWIDTH;

			for ( unsigned int column = startCol; column < static_cast<unsigned int>(endCol + 1); column++ )
			{
				uint8_t colAdj = 7 - ( column % 8 );
				unsigned int rowOffset = SH1106_LCDWIDTH / 8;

				uint8_t newByte = 0x0;
				newByte |= (( buffer[(column / 8) + pageOffset + (rowOffset * 0)] & (1 << colAdj) ) >> colAdj << 0);
				newByte |= (( buffer[(column / 8) + pageOffset + (rowOffset * 1)] & (1 << colAdj) ) >> colAdj << 1);
				newByte |= (( buffer[(column / 8) + pageOffset + (rowOffset * 2)] & (1 << colAdj) ) >> colAdj << 2);
				newByte |= (( buffer[(column / 8) + pageOffset + (rowOffset * 3)] & (1 << colAdj) ) >> colAdj << 3);
				newByte |= (( buffer[(column / 8) + pageOffset + (rowOffset * 4)] & (1 << colAdj) ) >> colAdj << 4);
				newByte |= (( buffer[(column / 8) + pageOffset + (rowOffset * 5)] & (1 << colAdj) ) >> colAdj << 5);
				newByte |= (( buffer[(column / 8) + pageOffset + (rowOffset * 6)] & (1 << colAdj) ) >> colAdj << 6);
				newByte |= (( buffer[(column / 8) + pageOffset + (rowOffset * 7)] & (1 << colAdj) ) >> colAdj << 7);

				this->sendData( newByte );
			}
		}
	}
}

void Oled_SH1106::reset()
{
	LLPD::gpio_output_set( m_RstPort, m_RstPin, true );

	// delay to ensure reset
	LLPD::tim6_delay( 10000 );

	LLPD::gpio_output_set( m_RstPort, m_RstPin, false );

	// delay to ensure reset
	LLPD::tim6_delay( 10000 );

	LLPD::gpio_output_set( m_RstPort, m_RstPin, true );

	// delay to ensure reset
	LLPD::tim6_delay( 10000 );
}

void Oled_SH1106::sendCommand (uint8_t command)
{
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );
	LLPD::gpio_output_set( m_DCPort, m_DCPin, false );
	LLPD::gpio_output_set( m_CSPort, m_CSPin, false );
	LLPD::spi_master_send_and_recieve( m_SpiNum, command );
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );
}

void Oled_SH1106::sendData (uint8_t data)
{
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );
	LLPD::gpio_output_set( m_DCPort, m_DCPin, true );
	LLPD::gpio_output_set( m_CSPort, m_CSPin, false );
	LLPD::spi_master_send_and_recieve( m_SpiNum, data );
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );
}
