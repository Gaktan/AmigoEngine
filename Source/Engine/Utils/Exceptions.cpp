#pragma once

#include "Engine.h"

#include "Utils/Logger.h"

#include <cstring>
#include <exception>
#include <math.h>

void ThrowIfFailed(uint32 inResult)
{
	if (inResult != 0)
		throw std::exception();
}

bool HandleAssert(const char* inFileName, int inLineNumber, const char* inExpression, const char* inMessage)
{
	Trace("******************************************************************************************************************************************************");
	Trace("*                                                            Assert failed                                                                           *");
	Trace("******************************************************************************************************************************************************");
	int expression_size = 142 - std::strlen(inFileName) - ::log10((float) inLineNumber);
	Trace("* %s(%d): %-*s *", inFileName, inLineNumber, expression_size, inExpression);
	if (inMessage != nullptr)
		Trace("* %-*s *", 146, inMessage);
	Trace("******************************************************************************************************************************************************");

	// Whether we should break or not
	return true;
}
