#pragma once

#include <Math.h>
#include <cmath>

#include <mathfu/quaternion.h>
#include <mathfu/matrix.h>
#include <mathfu/vector.h>

typedef mathfu::Vector<float, 2> Vec2;
typedef mathfu::Vector<float, 3> Vec3;
typedef mathfu::Vector<float, 4> Vec4;

typedef mathfu::Matrix<float, 4, 4> Mat4x4;

typedef mathfu::Quaternion<float> Quat;

namespace Math
{
	constexpr float Pi = 3.141592653589793238462643383279f;

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

	inline float Cos(float a)
	{
		return ::cosf(a);
	}

	inline float Tan(float a)
	{
		return ::tanf(a);
	}

	inline float Asin(float a)
	{
		return ::asinf(a);
	}

	inline float Acos(float a)
	{
		return ::acosf(a);
	}

	inline float Atan(float a)
	{
		return ::atanf(a);
	}

	inline float Atan2(float a, float b)
	{
		return ::atan2f(a, b);
	}

	inline float Sqrt(float a)
	{
		return ::sqrtf(a);
	}

	inline bool IsPowerOfTwo(int n)
	{
		return (::ceil(::log2(n)) == ::floor(::log2(n)));
	}
}
