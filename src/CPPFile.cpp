#include "CPPFile.hpp"

CPPFile::CPPFile (const std::string& fileName) :
	m_FileName( fileName ),
	m_NeedsInitialization( false )
{
	m_File.open( "./" + m_FileName, std::fstream::in | std::fstream::out | std::ios::binary );
	if ( !m_File.is_open() )
	{
		m_NeedsInitialization = true;
	}
}

CPPFile::~CPPFile()
{
	m_File.close();
}

void CPPFile::writeToMedia (const SharedData<uint8_t>& data, const unsigned int offsetInBytes)
{
	m_File.seekp( offsetInBytes );
	m_File.write( reinterpret_cast<char*>(data.getPtr()), data.getSizeInBytes() );
}

SharedData<uint8_t> CPPFile::readFromMedia (const unsigned int sizeInBytes, const unsigned int offsetInBytes)
{
	SharedData<uint8_t> data = SharedData<uint8_t>::MakeSharedData( sizeInBytes );

	m_File.seekg( offsetInBytes );
	m_File.read( reinterpret_cast<char*>(data.getPtr()), sizeInBytes );

	return data;
}

bool CPPFile::needsInitialization()
{
	return m_NeedsInitialization;
}

void CPPFile::initialize()
{
	m_File.open( "./" + m_FileName, std::fstream::out | std::ios::binary );
}

void CPPFile::afterInitialize()
{
	m_File.close();
	m_File.open( "./" + m_FileName, std::fstream::in | std::fstream::out | std::ios::binary );
}
