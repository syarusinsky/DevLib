#ifndef EEPROM_CAT24C64
#define EEPROM_CAT24C64

#include "LLPD.hpp"

class Eeprom_CAT24C64
{
	public:
		static const int EEPROM_SIZE = 8192;

		Eeprom_CAT24C64 (const I2C_NUM& i2cNum, bool A0IsHigh = false, bool A1IsHigh = false, bool A2IsHigh = false);
		~Eeprom_CAT24C64();

		void writeByte (uint16_t address, uint8_t data);
		uint8_t readByte (uint16_t address);

	private:
		uint8_t m_I2CAddress;
		I2C_NUM m_I2CNum;
};

#endif // CAT24C64_EEPROM
