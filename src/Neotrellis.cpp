#include "Neotrellis.hpp"

Neotrellis::Neotrellis (const I2C_NUM& i2cNum, uint8_t i2cAddr, const GPIO_PORT& intPort, const GPIO_PIN& intPin) :
	m_I2CNum( i2cNum ),
	m_I2CAddr( i2cAddr ),
	m_IntPort( intPort ),
	m_IntPin( intPin )
{
}

Neotrellis::~Neotrellis()
{
}

void Neotrellis::begin()
{
	LLPD::usart_log( USART_NUM::USART_3, "NEOTRELLIS INITIALIZING..." );

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
	LLPD::usart_log_int( USART_NUM::USART_3, "NEOTRELLIS HW ID : ", hwId1 );

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

	LLPD::usart_log( USART_NUM::USART_3, "NEOTRELLIS INITIALIZED" );
}

void Neotrellis::pollForEvents()
{
	if ( ! LLPD::gpio_input_get(m_IntPort, m_IntPin) )
	{
		LLPD::usart_log( USART_NUM::USART_3, "NEOTRELLIS KEYPAD INTERRUPT RECIEVED" );

		// set slave address
		LLPD::i2c_master_set_slave_address( m_I2CNum, I2C_ADDR_MODE::BITS_7, m_I2CAddr );

		// get event count
		LLPD::i2c_master_write( m_I2CNum, true, 2, SEESAW_KEYPAD_BASE, SEESAW_KEYPAD_COUNT );
		uint8_t eventCount = 0;
		LLPD::i2c_master_read( m_I2CNum, true, 1, &eventCount );
		LLPD::usart_log_int( USART_NUM::USART_3, "   KEYPAD EVENT COUNT : ", eventCount );

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
				LLPD::usart_log_int( USART_NUM::USART_3, "   ACTUAL VAL : ", eventBuffer[event] );

				bool keypadReleased = NEO_TRELLIS_KEY_RELEASED( eventBuffer[event] );
				if ( keypadReleased )
				{
					LLPD::usart_log( USART_NUM::USART_3, "   KEY RELEASED" );
					eventBuffer[event] += 1;
				}
				else
				{
					LLPD::usart_log( USART_NUM::USART_3, "   KEY PRESSED" );
				}

				uint8_t keyX = ( eventBuffer[event] / 4 ) % 4;
				uint8_t keyY = ( eventBuffer[event] / 32 );
				uint8_t keyNum = keyX + ( keyY * 4 );
				LLPD::usart_log_int( USART_NUM::USART_3, "   KEY NUM :", keyX + (keyY * 4) );
			}
		}
	}
}

void Neotrellis::test()
{
	LLPD::usart_log( USART_NUM::USART_3, "STARTING NEOTRELLIS TEST" );

	// define i2c addresses
	const uint16_t topLeftTileI2CAddr = 0x2F;
	// const uint16_t topRightTileI2CAddr = 0x2E;
	// const uint16_t bottomRightTileI2CAddr = 0x30;
	// const uint16_t bottomLeftTileI2CAddr = 0x31;

	// setup top left tile
	// set address
	LLPD::i2c_master_set_slave_address( I2C_NUM::I2C_2, I2C_ADDR_MODE::BITS_7, topLeftTileI2CAddr );

	// software reset
	LLPD::i2c_master_write( I2C_NUM::I2C_2, true, 3, SEESAW_STATUS_BASE, SEESAW_STATUS_SWRST, 0xFF );

	// get hardware id
	LLPD::i2c_master_write( I2C_NUM::I2C_2, false, 2, SEESAW_STATUS_BASE, SEESAW_STATUS_HW_ID );
	uint8_t hwId1 = 255;
	while ( hwId1 != SEESAW_NEOTRELLIS_HW_ID )
	{
		LLPD::i2c_master_read( I2C_NUM::I2C_2, true, 1, &hwId1 );
	}
	LLPD::usart_log_int( USART_NUM::USART_3, "SEESAW_STATUS_HW_ID : ", hwId1 );

	// set neopixel type
	LLPD::i2c_master_write( I2C_NUM::I2C_2, true, 3, SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_SPEED, 0x1 );

	// update neopixel length
	LLPD::i2c_master_write( I2C_NUM::I2C_2, true, 4, SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_BUF_LENGTH, 0x0, 0x30 );

	// set neopixel pin
	LLPD::i2c_master_write( I2C_NUM::I2C_2, true, 3, SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_PIN, 0x3 );

	// get hardware id TODO again for some reason? maybe we can get rid of this
	LLPD::i2c_master_write( I2C_NUM::I2C_2, false, 2, SEESAW_STATUS_BASE, SEESAW_STATUS_HW_ID );
	uint8_t hwId2 = 255;
	while ( hwId2 != SEESAW_NEOTRELLIS_HW_ID )
	{
		LLPD::i2c_master_read( I2C_NUM::I2C_2, true, 1, &hwId2 );
	}
	LLPD::usart_log_int( USART_NUM::USART_3, "SEESAW_STATUS_HW_ID : ", hwId2 );

	// enable keypad interrupt
	LLPD::i2c_master_write( I2C_NUM::I2C_2, true, 3, SEESAW_KEYPAD_BASE, SEESAW_KEYPAD_INTENSET, 0x01 );

	// set pixel color
	LLPD::i2c_master_write( I2C_NUM::I2C_2, true, 7, SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_BUF, 0x0, 0x24, 0xF, 0x0, 0x0 );
	LLPD::i2c_master_write( I2C_NUM::I2C_2, true, 2, SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_SHOW );

	LLPD::usart_log( USART_NUM::USART_3, "ENDING NEOTRELLIS TEST" );
}
