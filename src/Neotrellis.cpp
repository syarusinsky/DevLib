#include "Neotrellis.hpp"


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

void Neotrellis::begin()
{
	// set slave address
	LLPD::i2c_master_set_slave_address( m_I2CNum, I2C_ADDR_MODE::BITS_7, m_I2CAddr );

	// software reset
	LLPD::i2c_master_write( m_I2CNum, true, 3, SEESAW_STATUS_BASE, SEESAW_STATUS_SWRST, 0xFF );

	// get hardware id
	LLPD::i2c_master_write( m_I2CNum, true, 2, SEESAW_STATUS_BASE, SEESAW_STATUS_HW_ID );
	uint8_t hwId1 = 255;
	while ( hwId1 != SEESAW_NEOTRELLIS_HW_ID )
	{
		LLPD::i2c_master_read( m_I2CNum, true, 1, &hwId1 );
	}

	// set neopixel type
	LLPD::i2c_master_write( m_I2CNum, true, 3, SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_SPEED, 0x0 );

	// update neopixel length
	LLPD::i2c_master_write( m_I2CNum, true, 4, SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_BUF_LENGTH, 0x0, 0x30 );

	// set neopixel pin
	LLPD::i2c_master_write( m_I2CNum, true, 3, SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_PIN, 0x3 );

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

void Neotrellis::setColor (uint8_t keyRow, uint8_t keyCol, uint8_t r, uint8_t g, uint8_t b)
{
	// set slave address
	LLPD::i2c_master_set_slave_address( m_I2CNum, I2C_ADDR_MODE::BITS_7, m_I2CAddr );

	uint8_t keyVal = ( (keyCol * 4) + keyRow ) * 3;

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
				uint8_t keyNum = keyX + ( keyY * 4 );

				if ( m_Callbacks[keyNum] != nullptr )
				{
					m_Callbacks[keyNum]( this, keypadReleased, keyX, keyY );
				}
			}
		}
	}
}

void Neotrellis::registerCallback (uint8_t keyRow, uint8_t keyCol, NeotrellisCallback callback)
{
	m_Callbacks[ keyCol + (keyRow * NEO_TRELLIS_NUM_ROWS) ] = callback;
}
