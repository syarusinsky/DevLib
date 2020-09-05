#ifndef EEPROM_CAT24C64_HPP
#define EEPROM_CAT24C64_HPP

#include "LLPD.hpp"
#include "IStorageMedia.hpp"

class Eeprom_CAT24C64 : public IStorageMedia
{
	public:
		static const int EEPROM_SIZE = 8192;

		Eeprom_CAT24C64 (const I2C_NUM& i2cNum, bool A0IsHigh = false, bool A1IsHigh = false, bool A2IsHigh = false);
		~Eeprom_CAT24C64() override;

		void writeByte (uint16_t address, uint8_t data);
		uint8_t readByte (uint16_t address);

		void writeToMedia (const Variant& data, const unsigned int sizeInBytes, const unsigned int address) override;
		// NOTE!!! readFromMedia allocates memory, make sure to delete
		Variant readFromMedia (const unsigned int sizeInBytes, const unsigned int address) override;

		virtual bool needsInitialization() override { return false; }
		virtual void initialize() override {}
		virtual void afterInitialize() override {}

	private:
		uint8_t m_I2CAddress;
		I2C_NUM m_I2CNum;
};

#endif // CAT24C64_EEPROM_HPP
