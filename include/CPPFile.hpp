#ifndef CPPFILE_HPP
#define CPPFILE_HPP

/**************************************************************************
 * A CPPFile is a storage media that uses the C++ file utilities to write to
 * and read from a file.
**************************************************************************/

#include "IStorageMedia.hpp"
#include <fstream>

class CPPFile : public IStorageMedia
{
	public:
		CPPFile (const std::string& fileName);
		~CPPFile() override;

		void writeToMedia (const SharedData<uint8_t>& data, const unsigned int offsetInBytes) override;
		SharedData<uint8_t> readFromMedia (const unsigned int sizeInBytes, const unsigned int offsetInBytes) override;

		bool needsInitialization() override;
		void initialize() override;
		void afterInitialize() override;

	private:
		std::fstream m_File;
		std::string  m_FileName;
		bool         m_NeedsInitialization;
};

#endif // CPPFILE_HPP
