#ifndef OLED_SH1106_HPP
#define OLED_SH1106_HPP

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

class Oled_SH1106
{
	public:
		Oled_SH1106 (const SPI_NUM& spiNum, const GPIO_PORT& csPort, const GPIO_PIN& csPin, const GPIO_PORT& dcPort,
				const GPIO_PIN& dcPin, const GPIO_PORT& rstPort, const GPIO_PIN& rstPin);
		~Oled_SH1106();

		void begin();

		void displayFullRowMajor (uint8_t* buffer);

		void displayPartialRowMajor (uint8_t* buffer, uint8_t startRow, uint8_t startCol, uint8_t endRow, uint8_t endCol);

		void reset();

		void test();

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
