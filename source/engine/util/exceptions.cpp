#include "engine_precomp.h"
#include "exceptions.h"

#include <exception>
#include <cassert>

void ThrowIfFailed(ui32 hr)
{
	if (hr != 0)
	{
		throw std::exception();
	}
}

void Assert(bool test)
{
	assert(test);
}