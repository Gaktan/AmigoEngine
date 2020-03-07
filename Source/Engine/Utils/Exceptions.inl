#include "Engine.h"
#include "Exceptions.h"

#include "Utils/Logger.h"

#include <exception>
#include <cassert>

inline void ThrowIfFailed(uint32 inResult)
{
	if (inResult != 0)
	{
		throw std::exception();
	}
}

inline void Assert(bool inCondition)
{
	assert(inCondition);
}

inline void Assert(bool inCondition, const char* inMessage)
{
	if (!inCondition)
	{
		assert(inCondition);
		Trace(inMessage);
	}
}