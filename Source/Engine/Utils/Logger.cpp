#include "Engine.h"
#include "Utils/Logger.h"

#include <windows.h>

#define vsnprintf _vsnprintf_s

void Trace(const char* inMessage, /*args*/ ...)
{
	va_list args;
	va_start(args, inMessage);

	// Length of file after formatting
	size_t length	= _vscprintf(inMessage, args) + 1;

	// OutputDebugString has a maximum length of 4KB
	Assert(length < (4096-1));

	char* buffer	= new char[length+1];

	vsnprintf(buffer, length, length, inMessage, args);
	va_end(args);

	// Cause a flush
	buffer[length-1] = '\n';
	buffer[length] = '\0';

	OutputDebugString(buffer);

	delete[] buffer;
}