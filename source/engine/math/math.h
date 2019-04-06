#pragma once

// The min/max macros conflict with like-named member functions.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#include <math.h>
#include <cmath>

template<typename T>
inline T abs(T x)
{
	return fabs(x);
}

template<typename T>
inline T mod(T x, T y)
{
	return x - y * floor(x / y);
}

template<typename T>
inline T max(T a, T b)
{
	return (a > b) ? a : b;
}

template<typename T>
inline T min(T a, T b)
{
	return (a < b) ? a : b;
}

template<typename T>
inline T clamp(T f, T _min, T _max)
{
	return max(_min, min(_max, f));
}

template<typename T>
inline T sign(T x)
{
	return (x < 0) ? -1 : 1;
}

inline float toRadians(float x)
{
	return x * 0.01745329251f; // PI / 180
}

inline float toDegrees(float x)
{
	return x * 57.2957795131f; // 180 / PI
}

inline bool floatEquals(float a, float b, float epsilon = 1e-5)
{
	return abs(a - b) < epsilon;
}

inline bool isinf(float a)
{
	return !std::isfinite(a);
}

inline bool isnan(float a)
{
	return std::isnan(a);
}