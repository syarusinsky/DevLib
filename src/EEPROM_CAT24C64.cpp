#include "EEPROM_CAT24C64.hpp"

Eeprom_CAT24C64::Eeprom_CAT24C64 (const I2C_NUM& i2cNum, bool A0IsHigh, bool A1IsHigh, bool A2IsHigh) :
	m_I2CAddress( 0b01010000 ),
	m_I2CNum( i2cNum )
{
	if ( A0IsHigh )
	{
		m_I2CAddress |= 0b00000001;
	}

	if ( A1IsHigh )
	{
		m_I2CAddress |= 0b00000010;
	}

	if ( A2IsHigh )
	{
		m_I2CAddress |= 0b00000100;
	}
}

Eeprom_CAT24C64::~Eeprom_CAT24C64()
{
}

void Eeprom_CAT24C64::writeByte (uint16_t address, uint8_t data)
{
	// mask off any invalid bits to EEPROM address
	address &= 0b0001111111111111;

	// store address in two bytes
	uint8_t addrH = (address >> 8);
	uint8_t addrL = (address & 0b0000000011111111);

	// set llpd i2c address
	LLPD::i2c_master_set_slave_address( m_I2CNum, I2C_ADDR_MODE::BITS_7, m_I2CAddress );

	// send data
	LLPD::i2c_master_write( m_I2CNum, true, 3, addrH, addrL, data );
}

uint8_t Eeprom_CAT24C64::readByte (uint16_t address)
{
	// mask off any invalid bits to EEPROM address
	address &= 0b0001111111111111;

	// store address in two bytes
	uint8_t addrH = (address >> 8);
	uint8_t addrL = (address & 0b0000000011111111);

	// set llpd i2c address
	LLPD::i2c_master_set_slave_address( m_I2CNum, I2C_ADDR_MODE::BITS_7, m_I2CAddress );

	// send address bytes
	LLPD::i2c_master_write( m_I2CNum, false, 2, addrH, addrL );

	// read data
	uint8_t data = 0;
	LLPD::i2c_master_read( m_I2CNum, true, 1, &data );

	return data;
}

void Eeprom_CAT24C64::writeToMedia (const SharedData<uint8_t>& data, const unsigned int address)
{
	uint8_t* dataPtr = data.getPtr();

	for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
	{
		this->writeByte( address + byte, dataPtr[byte] );
	}
}

SharedData<uint8_t> Eeprom_CAT24C64::readFromMedia (const unsigned int sizeInBytes, const unsigned int address)
{
	SharedData<uint8_t> data = SharedData<uint8_t>::MakeSharedData( sizeInBytes );
	uint8_t* dataPtr = data.getPtr();

	for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
	{
		dataPtr[byte] = this->readByte( address + byte );
	}

	return data;
}

void Eeprom_CAT24C64::readFromMedia (const unsigned int address, const SharedData<uint8_t>& data)
{
	uint8_t* dataPtr = data.getPtr();

	for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
	{
		dataPtr[byte] = this->readByte( address + byte );
	}
}

Eeprom_CAT24C64_Manager::Eeprom_CAT24C64_Manager (const I2C_NUM& i2cNum, const std::vector<Eeprom_CAT24C64_AddressConfig>& addressConfigs) :
	m_Eeproms()
{
	for ( const Eeprom_CAT24C64_AddressConfig& addressConfig : addressConfigs )
	{
		m_Eeproms.emplace_back( i2cNum, addressConfig.m_A0IsHigh, addressConfig.m_A1IsHigh, addressConfig.m_A2IsHigh );
	}
}

void Eeprom_CAT24C64_Manager::writeByte (unsigned int address, uint8_t data)
{
	const unsigned int eepromNum = address / Eeprom_CAT24C64::EEPROM_SIZE;
	if ( eepromNum >= m_Eeproms.size() ) return;
	const uint16_t eepromAddress = address % Eeprom_CAT24C64::EEPROM_SIZE;

	m_Eeproms[eepromNum].writeByte( eepromAddress, data );
}

uint8_t Eeprom_CAT24C64_Manager::readByte (unsigned int address)
{
	const unsigned int eepromNum = address / Eeprom_CAT24C64::EEPROM_SIZE;
	if ( eepromNum >= m_Eeproms.size() ) return 0;
	const uint16_t eepromAddress = address % Eeprom_CAT24C64::EEPROM_SIZE;

	return m_Eeproms[eepromNum].readByte( eepromAddress );
}

void Eeprom_CAT24C64_Manager::writeToMedia (const SharedData<uint8_t>& data, const unsigned int address)
{
	uint8_t* dataPtr = data.getPtr();

	for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
	{
		this->writeByte( address + byte, dataPtr[byte] );
	}
}

SharedData<uint8_t> Eeprom_CAT24C64_Manager::readFromMedia (const unsigned int sizeInBytes, const unsigned int address)
{
	SharedData<uint8_t> data = SharedData<uint8_t>::MakeSharedData( sizeInBytes );
	uint8_t* dataPtr = data.getPtr();

	for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
	{
		dataPtr[byte] = this->readByte( address + byte );
	}

	return data;
}
