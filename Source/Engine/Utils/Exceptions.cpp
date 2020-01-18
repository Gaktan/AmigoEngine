#include "Engine.h"
#include "Exceptions.h"

#include <exception>
#include <cassert>

void ThrowIfFailed(uint32 inResult)
{
	if (inResult != 0)
	{
		throw std::exception();
	}
}

void Assert(bool inCondition)
{
	assert(inCondition);
}