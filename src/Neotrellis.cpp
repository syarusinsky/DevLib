#include "Neotrellis.hpp"

#include "LLPD.hpp"

#define noOp (void)0

Neotrellis::Neotrellis (const I2C_NUM& i2cNum, uint8_t i2cAddr, const GPIO_PORT& intPort, const GPIO_PIN& intPin) :
	m_I2CNum( i2cNum ),
	m_I2CAddr( i2cAddr ),
	m_IntPort( intPort ),
	m_IntPin( intPin ),
	m_Callbacks{ nullptr }
{
}

Neotrellis::~Neotrellis()
{
}

void Neotrellis::begin (NeotrellisListener* listener)
{
	NeotrellisInterface::begin( listener );

	// set slave address
	LLPD::i2c_master_set_slave_address( m_I2CNum, I2C_ADDR_MODE::BITS_7, m_I2CAddr );

	// software reset
	LLPD::i2c_master_write( m_I2CNum, true, 3, SEESAW_STATUS_BASE, SEESAW_STATUS_SWRST, 0xFF );

	// no op delay
	for ( unsigned int numNoOp = 0; numNoOp < 500000; numNoOp++ )
	{
		noOp;
	}

	// get hardware id
	uint8_t hwId1 = 0xFF;
	LLPD::i2c_master_write( m_I2CNum, true, 2, SEESAW_STATUS_BASE, SEESAW_STATUS_HW_ID );
	LLPD::i2c_master_read( m_I2CNum, true, 1, &hwId1 );

	// set neopixel type
	LLPD::i2c_master_write( m_I2CNum, true, 3, SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_SPEED, 0x01 );

	// update neopixel length
	LLPD::i2c_master_write( m_I2CNum, true, 4, SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_BUF_LENGTH, 0x0, 0x30 );

	// set neopixel pin
	LLPD::i2c_master_write( m_I2CNum, true, 3, SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_PIN, 0x03 );

	// ask for hardware id again for some reason? (arduino example code does this)
	LLPD::i2c_master_write( m_I2CNum, true, 2, SEESAW_STATUS_BASE, SEESAW_STATUS_HW_ID );
	LLPD::i2c_master_read( m_I2CNum, true, 1, &hwId1 );

	// enable keypad interrupt
	LLPD::i2c_master_write( m_I2CNum, true, 3, SEESAW_KEYPAD_BASE, SEESAW_KEYPAD_INTENSET, 0x01 );

	// enable keypad events
	for ( uint8_t key = 0; key < 16; key++ )
	{
		LLPD::i2c_master_write( m_I2CNum, true, 4, SEESAW_KEYPAD_BASE, SEESAW_KEYPAD_EVENT,
				NEO_TRELLIS_KEY(key), SEESAW_KEYPAD_EDGE_RISING );
		LLPD::i2c_master_write( m_I2CNum, true, 4, SEESAW_KEYPAD_BASE, SEESAW_KEYPAD_EVENT,
				NEO_TRELLIS_KEY(key), SEESAW_KEYPAD_EDGE_FALLING );
	}
}

void Neotrellis::setColor (uint8_t keyCol, uint8_t keyRow, uint8_t r, uint8_t g, uint8_t b)
{
	// set slave address
	LLPD::i2c_master_set_slave_address( m_I2CNum, I2C_ADDR_MODE::BITS_7, m_I2CAddr );

	uint8_t keyVal = ( (keyRow * 4) + keyCol ) * 3;

	// set color
	LLPD::i2c_master_write( m_I2CNum, true, 7, SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_BUF, 0x0, keyVal, g, r, b );

	// show neopixels
	LLPD::i2c_master_write( m_I2CNum, true, 2, SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_SHOW );
}

void Neotrellis::pollForEvents()
{
	if ( ! LLPD::gpio_input_get(m_IntPort, m_IntPin) )
	{
		// set slave address
		LLPD::i2c_master_set_slave_address( m_I2CNum, I2C_ADDR_MODE::BITS_7, m_I2CAddr );

		// get event count
		LLPD::i2c_master_write( m_I2CNum, true, 2, SEESAW_KEYPAD_BASE, SEESAW_KEYPAD_COUNT );
		uint8_t eventCount = 0;
		LLPD::i2c_master_read( m_I2CNum, true, 1, &eventCount );

		// sometimes event count can return 255, which is invalid
		if ( eventCount != 255 && eventCount > 0 )
		{
			// get keypad events
			uint8_t eventBuffer[ eventCount ];

			// read keypad fifo
			LLPD::i2c_master_write( m_I2CNum, true, 2, SEESAW_KEYPAD_BASE, SEESAW_KEYPAD_FIFO );
			LLPD::i2c_master_read_into_array( m_I2CNum, true, eventCount, eventBuffer );

			for ( int event = 0; event < eventCount; event++ )
			{
				bool keypadReleased = NEO_TRELLIS_KEY_RELEASED( eventBuffer[event] );
				if ( keypadReleased )
				{
					eventBuffer[event] += 1;
				}

				uint8_t keyX = ( eventBuffer[event] / 4 ) % 4;
				uint8_t keyY = ( eventBuffer[event] / 32 );
				uint8_t keyNum = keyX + ( keyY * NEO_TRELLIS_NUM_COLUMNS );

				if ( m_Callbacks[keyNum] != nullptr )
				{
					m_Callbacks[keyNum]( m_NeotrellisListener, this, keypadReleased, keyX, keyY );
				}
			}
		}
	}
}

void Neotrellis::registerCallback (uint8_t keyCol, uint8_t keyRow, NeotrellisCallback callback)
{
	m_Callbacks[ keyCol + (keyRow * NEO_TRELLIS_NUM_COLUMNS) ] = callback;
}

uint8_t Neotrellis::getStackedRowNumInMultitrellis (Multitrellis* multitrellis)
{
	Neotrellis** neotrellisArr = multitrellis->getNeotrellisArray();

	for ( unsigned int rowNum = 0; rowNum < multitrellis->getNumRows() / NEO_TRELLIS_NUM_ROWS; rowNum++ )
	{
		for ( unsigned int colNum = 0; colNum < multitrellis->getNumCols() / NEO_TRELLIS_NUM_COLUMNS; colNum++ )
		{
			if ( this == neotrellisArr[colNum + (rowNum * (multitrellis->getNumCols() / NEO_TRELLIS_NUM_COLUMNS))] ) return rowNum;
		}
	}

	return 255;
}

uint8_t Neotrellis::getStackedColNumInMultitrellis (Multitrellis* multitrellis)
{
	Neotrellis** neotrellisArr = multitrellis->getNeotrellisArray();

	for ( unsigned int rowNum = 0; rowNum < multitrellis->getNumRows() / NEO_TRELLIS_NUM_ROWS; rowNum++ )
	{
		for ( unsigned int colNum = 0; colNum < multitrellis->getNumCols() / NEO_TRELLIS_NUM_COLUMNS; colNum++ )
		{
			if ( this == neotrellisArr[colNum + (rowNum * (multitrellis->getNumCols() / NEO_TRELLIS_NUM_COLUMNS))] ) return colNum;
		}
	}

	return 255;
}

Multitrellis::Multitrellis (unsigned int stackedRows, unsigned int stackedColumns, const I2C_NUM& i2cNum, uint8_t i2cAddresses[],
				const GPIO_PORT& intPort, const GPIO_PIN& intPin) :
	m_NumStackedRows( stackedRows ),
	m_NumStackedCols( stackedColumns ),
	m_NeotrellisArr{ nullptr }
{
	for ( unsigned int neotrellis = 0; neotrellis < (stackedRows * stackedColumns); neotrellis++ )
	{
		m_NeotrellisArr[neotrellis] = new Neotrellis( i2cNum, i2cAddresses[neotrellis], intPort, intPin );
	}
}

Multitrellis::~Multitrellis()
{
	for ( unsigned int neotrellis = 0; neotrellis < (m_NumStackedRows * m_NumStackedCols); neotrellis++ )
	{
		delete m_NeotrellisArr[neotrellis];
	}
}

void Multitrellis::begin (NeotrellisListener* listener)
{
	for ( unsigned int neotrellis = 0; neotrellis < (m_NumStackedRows * m_NumStackedCols); neotrellis++ )
	{
		m_NeotrellisArr[neotrellis]->begin( listener );
	}
}

void Multitrellis::setColor (uint8_t keyCol, uint8_t keyRow, uint8_t r, uint8_t g, uint8_t b)
{
	if ( keyRow < (m_NumStackedRows * NEO_TRELLIS_NUM_ROWS) && keyCol < (m_NumStackedCols * NEO_TRELLIS_NUM_COLUMNS) )
	{
		unsigned int xTravel = keyCol / NEO_TRELLIS_NUM_COLUMNS;
		unsigned int yTravel = ( keyRow / NEO_TRELLIS_NUM_ROWS ) * m_NumStackedCols;
		unsigned int neotrellisOffset = xTravel + yTravel;

		m_NeotrellisArr[neotrellisOffset]->setColor( keyCol % NEO_TRELLIS_NUM_COLUMNS, keyRow % NEO_TRELLIS_NUM_ROWS,
								r, g, b );
	}
}

void Multitrellis::pollForEvents()
{
	for ( unsigned int neotrellis = 0; neotrellis < (m_NumStackedRows * m_NumStackedCols); neotrellis++ )
	{
		m_NeotrellisArr[neotrellis]->pollForEvents();
	}
}

void Multitrellis::registerCallback (uint8_t keyCol, uint8_t keyRow, NeotrellisCallback callback)
{
	if ( keyRow < (m_NumStackedRows * NEO_TRELLIS_NUM_ROWS) && keyCol < (m_NumStackedCols * NEO_TRELLIS_NUM_COLUMNS) )
	{
		unsigned int xTravel = keyCol / NEO_TRELLIS_NUM_COLUMNS;
		unsigned int yTravel = ( keyRow / NEO_TRELLIS_NUM_ROWS ) * m_NumStackedCols;
		unsigned int neotrellisOffset = xTravel + yTravel;

		m_NeotrellisArr[neotrellisOffset]->registerCallback( keyCol % NEO_TRELLIS_NUM_COLUMNS, keyRow % NEO_TRELLIS_NUM_ROWS,
									callback );
	}
}

#undef noOp
