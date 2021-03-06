#ifndef NEOTRELLIS_HPP
#define NEOTRELLIS_HPP

/**************************************************************************
 * A Neotrellis instance is an interface to an Adafruit Neotrellis 4x4
 * I2C board. It provides functions for initializing the board, polling
 * for button events, setting individual cell colors, and registering
 * callbacks to be called when polling for events.
 *
 * A Multitrellis instance is an interface to multiple tiled Adafruit
 * Neotrellis 4x4 I2C boards. It is possible to use multiple Neotrellis
 * instances to get the same functionality, but the Multitrellis class
 * makes this much easier.
**************************************************************************/

#include "LLPD.hpp"

#define NEO_TRELLIS_NUM_ROWS 4
#define NEO_TRELLIS_NUM_COLUMNS 4

#define NEO_TRELLIS_KEY(x) (((x) / 4) * 8 + ((x) % 4))
#define NEO_TRELLIS_KEY_RELEASED(x) ((x % 2) == 0)

#define NEO_TRELLIS_MAX_TILES 32

#define SEESAW_NEOTRELLIS_HW_ID 0x55
#define SEESAW_STATUS_BASE 0x00
#define SEESAW_STATUS_SWRST 0x7F
#define SEESAW_STATUS_HW_ID 0x01
#define SEESAW_NEOPIXEL_BASE 0x0E
#define SEESAW_NEOPIXEL_SPEED 0x02
#define SEESAW_NEOPIXEL_BUF_LENGTH 0x03
#define SEESAW_NEOPIXEL_PIN 0x01
#define SEESAW_NEOPIXEL_BUF 0x04
#define SEESAW_NEOPIXEL_SHOW 0x05
#define SEESAW_KEYPAD_BASE 0x10
#define SEESAW_KEYPAD_INTENSET 0x02
#define SEESAW_KEYPAD_EVENT 0x01
#define SEESAW_KEYPAD_EDGE_FALLING 0x9
#define SEESAW_KEYPAD_EDGE_RISING 0x11
#define SEESAW_KEYPAD_COUNT 0x04
#define SEESAW_KEYPAD_FIFO 0x10

class Neotrellis;

typedef void(*NeotrellisCallback)(Neotrellis*, bool, uint8_t, uint8_t); // bool is true if released, false if pressed.
									// the first uint8_t is the row, the second is the column

class Neotrellis
{
	public:
		Neotrellis (const I2C_NUM& i2cNum, uint8_t i2cAddr, const GPIO_PORT& intPort, const GPIO_PIN& intPin);
		~Neotrellis();

		void begin();

		void setColor (uint8_t keyRow, uint8_t keyCol, uint8_t r, uint8_t g, uint8_t b);

		void pollForEvents();

		void registerCallback (uint8_t keyRow, uint8_t keyCol, NeotrellisCallback callback);

	private:
		const I2C_NUM   	m_I2CNum;
		const uint8_t   	m_I2CAddr;
		const GPIO_PORT 	m_IntPort;
		const GPIO_PIN  	m_IntPin;
		NeotrellisCallback 	m_Callbacks[NEO_TRELLIS_NUM_ROWS * NEO_TRELLIS_NUM_COLUMNS];
};

class Multitrellis
{
	public:
		// you need to pass in the i2c addresses in row-major order
		Multitrellis (unsigned int stackedRows, unsigned int stackedColumns, const I2C_NUM& i2cNum, uint8_t i2cAddresses[],
				const GPIO_PORT& intPort, const GPIO_PIN& intPin);
		~Multitrellis();

		void begin();

		void setColor (unsigned int keyRow, unsigned int keyCol, uint8_t r, uint8_t g, uint8_t b);

		void pollForEvents();

		void registerCallback (unsigned int keyRow, unsigned int keyCol, NeotrellisCallback callback);

	private:
		const unsigned int m_NumStackedRows;
		const unsigned int m_NumStackedCols;
		Neotrellis*  m_NeotrellisArr[NEO_TRELLIS_MAX_TILES]; // tiles are stored in row major order
};

#endif // NEOTRELLIS_HPP
