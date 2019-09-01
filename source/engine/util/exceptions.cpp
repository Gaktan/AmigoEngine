#include "engine_precomp.h"
#include "exceptions.h"

#include <exception>
#include <cassert>

void ThrowIfFailed(ui32 inResult)
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