#include "Engine.h"
#include "FileReader.h"

#include <fstream>

FileReader::~FileReader()
{
	if (m_FileContent)
	{
		delete[] m_FileContent;
	}
}

bool FileReader::ReadFile(const std::string& inFilename)
{
	// Seek to the end of stream immediately after open
	std::ifstream stream(inFilename, std::ios::binary | std::ios::ate);
	if (!stream.is_open())
	{
		// Failed to open file
		Assert(false);
		return false;
	}

	std::streamsize size = stream.tellg();
	stream.seekg(0, std::ios::beg);

	// Need to add 1 for \0
	m_FileContent = new char[size + 1LL];

	if (!stream.read(m_FileContent, size))
	{
		// Failed to read from file...
		Assert(false);
		delete[] m_FileContent;
		m_FileContent = nullptr;
		return false;
	}

	m_FileContent[size] = '\0';

	return true;
}


const char* FileReader::GetContentAsString() const
{
	return (char*) m_FileContent;
}

const void* FileReader::GetContentAsBinary() const
{
	return (void*) m_FileContent;
}