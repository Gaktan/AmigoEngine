#pragma once

#include <math.h>
#include <cmath>

namespace Math
{
	template<typename T>
	inline T Abs(T x)
	{
		return ::fabs(x);
	}

	template<typename T>
	inline T Mod(T x, T y)
	{
		return x - y * ::floor(x / y);
	}

	template<typename T>
	inline T Max(T a, T b)
	{
		return (a > b) ? a : b;
	}

	template<typename T>
	inline T Min(T a, T b)
	{
		return (a < b) ? a : b;
	}

	template<typename T>
	inline T Clamp(T f, T _min, T _max)
	{
		return Max(_min, Min(_max, f));
	}

	template<typename T>
	inline T Sign(T x)
	{
		return (x < 0) ? -1 : 1;
	}

	inline float ToRadians(float x)
	{
		return x * 0.01745329251f; // PI / 180
	}

	inline float ToDegrees(float x)
	{
		return x * 57.2957795131f; // 180 / PI
	}

	inline bool FloatEquals(float a, float b, float epsilon = 1e-5)
	{
		return Abs(a - b) < epsilon;
	}

	inline bool IsInf(float a)
	{
		return !std::isfinite(a);
	}

	inline bool IsNaN(float a)
	{
		return std::isnan(a);
	}

	inline float Sin(float a)
	{
		return ::sinf(a);
	}
}
