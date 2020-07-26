#ifndef SRAM_23K256
#define SRAM_23K256

#include "LLPD.hpp"

class Sram_23K256
{
	public:
		static const int SRAM_SIZE = 65536;

		Sram_23K256 (const SPI_NUM& spiNum, const GPIO_PORT& csPort, const GPIO_PIN& csPin);
		~Sram_23K256();

		void writeByte (uint16_t address, uint8_t data);
		uint8_t readByte (uint16_t address);

	private:
		SPI_NUM   m_SpiNum;
		GPIO_PORT m_CSPort; // chip select pin port
		GPIO_PIN  m_CSPin;
};

#endif // SRAM_23K256
