#ifndef SRAM_23K256_HPP
#define SRAM_23K256_HPP

#include "LLPD.hpp"
#include "IStorageMedia.hpp"

class Sram_23K256 : public IStorageMedia
{
	public:
		static const int SRAM_SIZE = 65536;

		Sram_23K256 (const SPI_NUM& spiNum, const GPIO_PORT& csPort, const GPIO_PIN& csPin);
		~Sram_23K256();

		void writeByte (uint16_t address, uint8_t data);
		uint8_t readByte (uint16_t address);

		void writeToMedia (const Variant& data, const unsigned int sizeInBytes, const unsigned int address) override;
		// NOTE!!! readFromMedia allocates memory, make sure to delete
		Variant readFromMedia (const unsigned int sizeInBytes, const unsigned int address) override;

		virtual bool needsInitialization() override { return false; }
		virtual void initialize() override {}
		virtual void afterInitialize() override {}

	private:
		SPI_NUM   m_SpiNum;
		GPIO_PORT m_CSPort; // chip select pin port
		GPIO_PIN  m_CSPin;
};

#endif // SRAM_23K256_HPP
