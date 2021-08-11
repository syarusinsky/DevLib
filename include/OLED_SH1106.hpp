#ifndef OLED_SH1106_HPP
#define OLED_SH1106_HPP

/**************************************************************************
 * A Oled_SH1106 instance is an interface to a Sino Wealth SH1106 128x64
 * pixel OLED display. It provides functions for initializing the display,
 * displaying a full frame buffer, displaying a partial frame buffer, and
 * reseting the display.
**************************************************************************/

#include "LLPD.hpp"

#define SH1106_LCDBUFFERWIDTH 		132
#define SH1106_LCDWIDTH 		128
#define SH1106_LCDHEIGHT 		64
#define SH1106_NUMPAGES 		8
#define SH1106_SETCONTRAST 		0x81
#define SH1106_SETNORMALDISPLAY 	0xA6
#define SH1106_SETREVERSEDISPLAY 	0xA7
#define SH1106_SETDISPLAYOFF 		0xAE
#define SH1106_SETDISPLAYON 		0xAF
#define SH1106_SETCHARGEPUMP6_4V 	0x30
#define SH1106_SETCHARGEPUMP7_4V 	0x31
#define SH1106_SETCHARGEPUMP8V 		0x32
#define SH1106_SETCHARGEPUMP9V 		0x33
#define SH1106_SETDISPLAYOFFSET 	0xD3
#define SH1106_SETCOMPINS 		0xDA
#define SH1106_SETVCOMDETECT 		0xDB
#define SH1106_SETDISPLAYCLOCKDIV 	0xD5
#define SH1106_SETPRECHARGE 		0xD9
#define SH1106_SETMULTIPLEX 		0xA8
#define SH1106_SETSEGREMAPRIGHT 	0xA1
#define SH1106_SETSEGREMAPLEFT 		0xA0
#define SH1106_SETLOWCOLUMN 		0x00
#define SH1106_SETHIGHCOLUMN 		0x10
#define SH1106_SETPAGEADDRESS           0xB0
#define SH1106_SETSTARTLINE 		0x40
#define SH1106_SETCOMSCANINC 		0xC0
#define SH1106_SETCOMSCANDEC 		0xC8

enum class REFRESH_RATE_PRESCALE : unsigned int
{
	BY_1 = 0,
	BY_2,
	BY_3,
	BY_4,
	BY_5,
	BY_6,
	BY_7,
	BY_8,
	BY_9,
	BY_10,
	BY_11,
	BY_12,
	BY_13,
	BY_14,
	BY_15,
	BY_16
};

class Oled_SH1106
{
	public:
		Oled_SH1106 (const SPI_NUM& spiNum, const GPIO_PORT& csPort, const GPIO_PIN& csPin, const GPIO_PORT& dcPort,
				const GPIO_PIN& dcPin, const GPIO_PORT& rstPort, const GPIO_PIN& rstPin);
		~Oled_SH1106();

		void begin();

		void displayFullRowMajor (uint8_t* buffer);

		void displayPartialRowMajor (uint8_t* buffer, uint8_t startRow, uint8_t startCol, uint8_t endRow, uint8_t endCol);

		void setRefreshRatePrescaler (const REFRESH_RATE_PRESCALE& presc);

		void reset();

	private:
		SPI_NUM   m_SpiNum;
		GPIO_PORT m_CSPort; // chip select pin port
		GPIO_PIN  m_CSPin;
		GPIO_PORT m_DCPort; // data/command pin port
		GPIO_PIN  m_DCPin;
		GPIO_PORT m_RstPort; // reset pin port
		GPIO_PIN  m_RstPin;

		void sendCommand (uint8_t command);
		void sendData (uint8_t data);
};

#endif // OLED_SH1106_HPP
