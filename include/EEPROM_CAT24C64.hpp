#ifndef EEPROM_CAT24C64_HPP
#define EEPROM_CAT24C64_HPP

/**************************************************************************
 * An Eeprom_CAT24C64 instance is an interface to an ON Semiconductor
 * CAT24C64 64Kb I2C EEPROM. It provides functions to write individual
 * bytes as well as larger chunks of data.
**************************************************************************/

#include "LLPD.hpp"
#include "IStorageMedia.hpp"

#include <vector>

class Eeprom_CAT24C64 : public IStorageMedia
{
	public:
		static const int EEPROM_SIZE = 8192;

		Eeprom_CAT24C64 (const I2C_NUM& i2cNum, bool A0IsHigh = false, bool A1IsHigh = false, bool A2IsHigh = false, bool hasMBR = false);
		~Eeprom_CAT24C64() override;

		void writeByte (uint16_t address, uint8_t data);
		uint8_t readByte (uint16_t address);

		void writeToMedia (const SharedData<uint8_t>& data, const unsigned int address) override;
		SharedData<uint8_t> readFromMedia (const unsigned int sizeInBytes, const unsigned int address) override;

		virtual bool needsInitialization() override { return false; }
		virtual void initialize() override {}
		virtual void afterInitialize() override {}

		virtual bool hasMBR() override { return m_HasMBR; }

	private:
		uint8_t m_I2CAddress;
		I2C_NUM m_I2CNum;
		bool 	m_HasMBR;
};

struct Eeprom_CAT24C64_AddressConfig
{
	bool m_A0IsHigh;
	bool m_A1IsHigh;
	bool m_A2IsHigh;

	Eeprom_CAT24C64_AddressConfig (bool A0IsHigh, bool A1IsHigh, bool A2IsHigh) :
		m_A0IsHigh( A0IsHigh ),
		m_A1IsHigh( A1IsHigh ),
		m_A2IsHigh( A2IsHigh ) {}
};

// a simple class to manage more than one CAT24C64 on the same I2C bus
class Eeprom_CAT24C64_Manager : public IStorageMedia
{
	public:
		Eeprom_CAT24C64_Manager (const I2C_NUM& i2cNum, const std::vector<Eeprom_CAT24C64_AddressConfig>& addressConfigs);

		void writeByte (unsigned int address, uint8_t data);
		uint8_t readByte (unsigned int address);

		void writeToMedia (const SharedData<uint8_t>& data, const unsigned int address) override;
		SharedData<uint8_t> readFromMedia (const unsigned int sizeInBytes, const unsigned int address) override;

		bool needsInitialization() override { return false; }
		void initialize() override {}
		void afterInitialize() override {}

		bool hasMBR() override { return false; }

	private:
		std::vector<Eeprom_CAT24C64> m_Eeproms;
};

#endif // CAT24C64_EEPROM_HPP
