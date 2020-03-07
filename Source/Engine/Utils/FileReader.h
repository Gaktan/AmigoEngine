#pragma once

#include <string>

class FileReader
{
protected:
	char* m_FileContent = nullptr;

public:
	virtual ~FileReader();

	bool			ReadFile(const std::string& inFilename);

	const char*		GetContentAsString() const;
	const void*		GetContentAsBinary() const;
};