#pragma once

#include <string>

class FileReader final
{
public:
	~FileReader();

	bool			ReadFile(const std::string& inFilename);

	const char*		GetContentAsString() const;
	const void*		GetContentAsBinary() const;
	uint64			GetContentSize() const;

protected:
	char*	m_FileContent = nullptr;
	uint64	m_ContentSize;
};