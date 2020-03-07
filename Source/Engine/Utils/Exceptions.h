#pragma once

void ThrowIfFailed(uint32 inResult);
void Assert(bool inCondition);
void Assert(bool inCondition, const char* inMessage);

#include "Utils/Exceptions.inl"