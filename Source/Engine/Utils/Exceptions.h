#pragma once

void ThrowIfFailed(uint32 inResult);
#define Assert(expression, ...) do { if (!(expression) && HandleAssert(__FILE__, __LINE__, #expression, ##__VA_ARGS__)) __debugbreak(); } while (0);

bool HandleAssert(const char* inFileName, int inLineNumber, const char* inExpression, const char* inMessage = nullptr);
