#pragma once

template <typename T>
class Vector4
{
protected:
	T data[4];

public:
	Vector4(T x = 0);
	Vector4(T x, T y, T z, T w = 0);
	Vector4(const Vector4<T>& inOther);

	void Set(T x = 0);
	void Set(T x, T y, T z, T w = 0);

	T X() const;
	T Y() const;
	T Z() const;
	T W() const;
	T& X();
	T& Y();
	T& Z();
	T& W();

	T& operator[](size_t inIndex);
	const T& operator[](size_t inIndex) const;

	Vector4<T> operator + (T x) const;

	Vector4<T> operator + (const Vector4<T>& inOther) const;
	Vector4<T> operator - (T x) const;
	Vector4<T> operator - (const Vector4<T>& inOther) const;
	Vector4<T> operator - () const;
	Vector4<T> operator * (T x) const;
	Vector4<T> operator / (T x) const;

	static T			Dot(const Vector4<T>& v0, const Vector4<T>& v1);
	static Vector4<T>	Cross(const Vector4<T>& v0, const Vector4<T>& v1);

	T LengthSquared() const;
	T Length() const;

	static Vector4<T> Normalize(const Vector4<T>& v);
};

#include "Math/Vec4.inl"

typedef Vector4<float> Vec4;
typedef Vector4<int32> IVec4;