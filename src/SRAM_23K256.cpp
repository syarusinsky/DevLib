#include "SRAM_23K256.hpp"

Sram_23K256::Sram_23K256 (const SPI_NUM& spiNum, const GPIO_PORT& csPort, const GPIO_PIN& csPin) :
	m_SpiNum( spiNum ),
	m_CSPort( csPort ),
	m_CSPin( csPin ),
	m_SequentialMode( false )
{
	// these srams seem to write incorrect data for the first couple of cycles, so lets get this over with
	uint16_t addressToReadAndWriteFrom = 63;
	uint8_t dataToWrite = 17;
	uint8_t dataRead = 0;
	while ( dataRead != dataToWrite )
	{
		LLPD::tim6_delay ( 1000 );
		this->writeByte( addressToReadAndWriteFrom, dataToWrite );
		LLPD::tim6_delay ( 1000 );
		dataRead = this->readByte( addressToReadAndWriteFrom );
	}
}

Sram_23K256::~Sram_23K256()
{
}

uint8_t Sram_23K256::readStatusRegister()
{
	// pull cs low
	LLPD::gpio_output_set( m_CSPort, m_CSPin, false );

	// send read status register instruction
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0b00000101 );

	// recieve status register value
	uint8_t statusRegVal = LLPD::spi_master_send_and_recieve( m_SpiNum, 0b00000000 );

	// pull cs high
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );

	return statusRegVal;
}

void Sram_23K256::writeStatusRegister (uint8_t regVal)
{
	// pull cs low
	LLPD::gpio_output_set( m_CSPort, m_CSPin, false );

	// send write status register instruction
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0b00000001 );

	// send register value
	LLPD::spi_master_send_and_recieve( m_SpiNum, regVal );

	// pull cs high
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );
}

bool Sram_23K256::setSequentialMode (bool sequential)
{
	uint8_t statusRegVal = this->readStatusRegister();

	// clear mode bits, which sets the mode to byte mode
	statusRegVal &= ~( 0b11000000 );

	if ( sequential )
	{
		// set bits to set to sequential mode
		statusRegVal |= 0b01000000;

		m_SequentialMode = true;
	}
	else
	{
		m_SequentialMode = false;
	}

	this->writeStatusRegister( statusRegVal );

	// verify that status register was set correctly
	if ( this->readStatusRegister() != statusRegVal )
	{
		return false;
	}

	return true;
}

bool Sram_23K256::getSequentialMode()
{
	return m_SequentialMode;
}

void Sram_23K256::writeByte (uint16_t address, uint8_t data)
{
	// pull cs low
	LLPD::gpio_output_set( m_CSPort, m_CSPin, false );

	// send write instruction
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0b00000010 );

	// send first half of address
	LLPD::spi_master_send_and_recieve( m_SpiNum, (address >> 8) );

	// send second half of address
	LLPD::spi_master_send_and_recieve( m_SpiNum, (address & 0b0000000011111111) );

	// send data
	LLPD::spi_master_send_and_recieve( m_SpiNum, data );

	// pull cs high
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );
}

uint8_t Sram_23K256::readByte (uint16_t address)
{
	// pull cs low
	LLPD::gpio_output_set( m_CSPort, m_CSPin, false );

	// send read instruction
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0b00000011 );

	// send first half of address
	LLPD::spi_master_send_and_recieve( m_SpiNum, (address >> 8) );

	// send second half of address
	LLPD::spi_master_send_and_recieve( m_SpiNum, (address & 0b0000000011111111) );

	// receive data
	uint8_t data = LLPD::spi_master_send_and_recieve( m_SpiNum, 0b00000000 );

	// pull cs high
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );

	return data;
}

void Sram_23K256::writeSequentialBytes (uint16_t startAddress, const SharedData<uint8_t>& data)
{
	// pull cs low
	LLPD::gpio_output_set( m_CSPort, m_CSPin, false );

	// send write instruction
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0b00000010 );

	// send first half of address
	LLPD::spi_master_send_and_recieve( m_SpiNum, (startAddress >> 8) );

	// send second half of address
	LLPD::spi_master_send_and_recieve( m_SpiNum, (startAddress & 0b0000000011111111) );

	for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
	{
		// send data
		LLPD::spi_master_send_and_recieve( m_SpiNum, data[byte] );
	}

	// pull cs high
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );
}

SharedData<uint8_t> Sram_23K256::readSequentialBytes (uint16_t startAddress, unsigned int sizeInBytes)
{
	// pull cs low
	LLPD::gpio_output_set( m_CSPort, m_CSPin, false );

	// send read instruction
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0b00000011 );

	// send first half of address
	LLPD::spi_master_send_and_recieve( m_SpiNum, (startAddress >> 8) );

	// send second half of address
	LLPD::spi_master_send_and_recieve( m_SpiNum, (startAddress & 0b0000000011111111) );

	const SharedData<uint8_t> data = SharedData<uint8_t>::MakeSharedData( sizeInBytes );

	for ( unsigned int byte = 0; byte < sizeInBytes; byte++ )
	{
		// read data
		data[byte] = LLPD::spi_master_send_and_recieve( m_SpiNum, 0b00000000 );
	}

	// pull cs high
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );

	return data;
}

void Sram_23K256::readSequentialBytes (uint16_t startAddress, const SharedData<uint8_t>& data)
{
	// pull cs low
	LLPD::gpio_output_set( m_CSPort, m_CSPin, false );

	// send read instruction
	LLPD::spi_master_send_and_recieve( m_SpiNum, 0b00000011 );

	// send first half of address
	LLPD::spi_master_send_and_recieve( m_SpiNum, (startAddress >> 8) );

	// send second half of address
	LLPD::spi_master_send_and_recieve( m_SpiNum, (startAddress & 0b0000000011111111) );

	for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
	{
		// read data
		data[byte] = LLPD::spi_master_send_and_recieve( m_SpiNum, 0b00000000 );
	}

	// pull cs high
	LLPD::gpio_output_set( m_CSPort, m_CSPin, true );
}

void Sram_23K256::writeToMedia (const SharedData<uint8_t>& data, const unsigned int address)
{
	if ( m_SequentialMode )
	{
		this->writeSequentialBytes( address, data );
	}
	else
	{
		uint8_t* dataPtr = data.getPtr();

		for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
		{
			this->writeByte( address + byte, dataPtr[byte] );
		}
	}
}

SharedData<uint8_t> Sram_23K256::readFromMedia (const unsigned int sizeInBytes, const unsigned int address)
{

	if ( m_SequentialMode )
	{
		return this->readSequentialBytes( address, sizeInBytes );
	}
	else
	{
		const SharedData<uint8_t> data = SharedData<uint8_t>::MakeSharedData( sizeInBytes );
		uint8_t* dataPtr = data.getPtr();

		for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
		{
			dataPtr[byte] = this->readByte( address + byte );
		}

		return data;
	}
}

void Sram_23K256::readFromMedia (const unsigned int address, const SharedData<uint8_t>& data)
{
	if ( m_SequentialMode )
	{
		this->readSequentialBytes( address, data );
	}
	else
	{
		uint8_t* dataPtr = data.getPtr();

		for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
		{
			dataPtr[byte] = this->readByte( address + byte );
		}
	}
}

Sram_23K256_Manager::Sram_23K256_Manager (const SPI_NUM& spiNum, const std::vector<Sram_23K256_GPIO_Config>& gpioConfigs) :
	m_Srams()
{
	for ( const Sram_23K256_GPIO_Config& gpioConfig : gpioConfigs )
	{
		LLPD::gpio_output_setup( gpioConfig.m_CSPort, gpioConfig.m_CSPin, GPIO_PUPD::PULL_UP, GPIO_OUTPUT_TYPE::PUSH_PULL,
						GPIO_OUTPUT_SPEED::HIGH, false );
		m_Srams.emplace_back( spiNum, gpioConfig.m_CSPort, gpioConfig.m_CSPin );
	}
}

bool Sram_23K256_Manager::setSequentialMode (bool sequential)
{
	bool successful = true;

	for ( Sram_23K256& sram : m_Srams )
	{
		if ( ! sram.setSequentialMode(sequential) )
		{
			successful = false;
		}
	}

	return successful;
}

void Sram_23K256_Manager::writeByte (uint32_t address, uint8_t data)
{
	unsigned int sramNum = address / Sram_23K256::SRAM_SIZE;
	if ( sramNum >= m_Srams.size() ) return; // ensure sram exists
	unsigned int sramAddress = address % Sram_23K256::SRAM_SIZE;

	m_Srams[sramNum].writeByte( sramAddress, data );
}

uint8_t Sram_23K256_Manager::readByte (uint32_t address)
{
	unsigned int sramNum = address / Sram_23K256::SRAM_SIZE;
	if ( sramNum >= m_Srams.size() ) return 0; // ensure sram exists
	unsigned int sramAddress = address % Sram_23K256::SRAM_SIZE;

	uint8_t retVal = m_Srams[sramNum].readByte( sramAddress );

	return retVal;
}

void Sram_23K256_Manager::writeSequentialBytes (unsigned int startAddress, const SharedData<uint8_t>& data)
{
	unsigned int dataIndex = 0;
	for ( unsigned int sramNum = 0; sramNum < m_Srams.size(); sramNum++ )
	{
		this->writeSequentialBytesHelper( startAddress, data, sramNum, dataIndex );
	}
}

SharedData<uint8_t> Sram_23K256_Manager::readSequentialBytes (unsigned int startAddress, unsigned int sizeInBytes)
{
	const SharedData<uint8_t> retData = SharedData<uint8_t>::MakeSharedData( sizeInBytes );
	unsigned int retDataIndex = 0;

	for ( unsigned int sramNum = 0; sramNum < m_Srams.size(); sramNum++ )
	{
		this->readSequentialBytesHelper( startAddress, retData, sramNum, retDataIndex );
	}

	return retData;
}

void Sram_23K256_Manager::readSequentialBytes (unsigned int startAddress, const SharedData<uint8_t>& data)
{
	unsigned int retDataIndex = 0;

	for ( unsigned int sramNum = 0; sramNum < m_Srams.size(); sramNum++ )
	{
		this->readSequentialBytesHelper( startAddress, data, sramNum, retDataIndex );
	}
}

void Sram_23K256_Manager::writeToMedia (const SharedData<uint8_t>& data, const unsigned int address)
{
	if ( m_Srams[0].getSequentialMode() )
	{
		this->writeSequentialBytes( address, data );
	}
	else
	{
		uint8_t* dataPtr = data.getPtr();

		for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
		{
			this->writeByte( address + byte, dataPtr[byte] );
		}
	}
}

SharedData<uint8_t> Sram_23K256_Manager::readFromMedia (const unsigned int sizeInBytes, const unsigned int address)
{
	if ( m_Srams[0].getSequentialMode() )
	{
		return this->readSequentialBytes( address, sizeInBytes );
	}
	else
	{
		const SharedData<uint8_t> data = SharedData<uint8_t>::MakeSharedData( sizeInBytes );
		uint8_t* dataPtr = data.getPtr();

		for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
		{
			dataPtr[byte] = this->readByte( address + byte );
		}

		return data;
	}
}

void Sram_23K256_Manager::readFromMedia (const unsigned int address, const SharedData<uint8_t>& data)
{
	if ( m_Srams[0].getSequentialMode() )
	{
		this->readSequentialBytes( address, data );
	}
	else
	{
		uint8_t* dataPtr = data.getPtr();

		for ( unsigned int byte = 0; byte < data.getSizeInBytes(); byte++ )
		{
			dataPtr[byte] = this->readByte( address + byte );
		}
	}
}

unsigned int Sram_23K256_Manager::clipStartAddress (unsigned int startAddress, unsigned int sizeInBytes, unsigned int sramNum)
{
	const unsigned int sramSize = Sram_23K256::SRAM_SIZE; // just to shorten variable names

	unsigned int retVal = startAddress;
	if ( startAddress < (sramSize * sramNum) ) { retVal = (sramSize * sramNum); } // this will be a valid index
	else if ( startAddress >= (sramSize * (sramNum + 1)) ) { retVal = (sramSize * (sramNum + 1)); } // this will not

	return retVal;
}

unsigned int Sram_23K256_Manager::clipEndAddress (unsigned int endAddress, unsigned int sizeInBytes, unsigned int sramNum)
{
	const unsigned int sramSize = Sram_23K256::SRAM_SIZE; // just to shorten variable names

	unsigned int retVal = endAddress;
	if ( endAddress < (sramSize * sramNum) ) { retVal = (sramSize * (sramNum + 1)); } // this will be an invalid index
	else if ( endAddress >= (sramSize * (sramNum + 1)) ) { retVal = (sramSize * (sramNum + 1)) - 1; } // this will not

	return retVal;
}

void Sram_23K256_Manager::writeSequentialBytesHelper (unsigned int startAddress, const SharedData<uint8_t>& data, unsigned int sramNum,
							unsigned int& dataIndex)
{
	// just to shorten variable names and make things a bit more readable
	const unsigned int sramSize = Sram_23K256::SRAM_SIZE;
	const unsigned int sizeInBytes = data.getSizeInBytes();
	const unsigned int totalEndAddress = startAddress + sizeInBytes - 1;

	const unsigned int sramStart = this->clipStartAddress( startAddress, sizeInBytes, sramNum );
	const unsigned int sramEnd = this->clipEndAddress( totalEndAddress, sizeInBytes, sramNum );

	// check for invalid indices
	if ( sramStart != (sramSize * (sramNum + 1)) && sramEnd != (sramSize * (sramNum + 1)) )
	{
		const unsigned int sizeToWrite = ( sramEnd - sramStart ) + 1;
		const SharedData<uint8_t> dataFragment = SharedData<uint8_t>::MakeSharedData( sizeToWrite, data.getPtr() + dataIndex );
		m_Srams[sramNum].writeSequentialBytes( sramStart, dataFragment );
		dataIndex += sizeToWrite;
	}
}

void Sram_23K256_Manager::readSequentialBytesHelper (unsigned int startAddress, const SharedData<uint8_t>& data, unsigned int sramNum,
							unsigned int& dataIndex)
{
	// just to shorted variable names and make things a bit more readable
	const unsigned int sramSize = Sram_23K256::SRAM_SIZE;
	const unsigned int sizeInBytes = data.getSizeInBytes();
	const unsigned int totalEndAddress = startAddress + sizeInBytes - 1;

	const unsigned int sramStart = this->clipStartAddress( startAddress, sizeInBytes, sramNum );
	const unsigned int sramEnd = this->clipEndAddress( totalEndAddress, sizeInBytes, sramNum );

	// check for invalid indices
	if ( sramStart != (sramSize * (sramNum + 1)) && sramEnd != (sramSize * (sramNum + 1)) )
	{
		const unsigned int sizeToRead = ( sramEnd - sramStart ) + 1;
		m_Srams[sramNum].readSequentialBytes( sramStart, data );
		dataIndex += sizeToRead;
	}
}
