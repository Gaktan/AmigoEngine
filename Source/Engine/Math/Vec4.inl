#pragma once

#include "Math/Vec4.h"

template <typename T>
Vector4<T>::Vector4(T x/* = 0*/)
{
	Set(x);
}

template <typename T>
Vector4<T>::Vector4(T x, T y, T z, T w/* = 0*/)
{
	Set(x, y, z, w);
}

template <typename T>
Vector4<T>::Vector4(const Vector4<T>& inOther)
{
	::memcpy(&data[0], &inOther.data[0], sizeof(T) * 4);
}

template <typename T>
void Vector4<T>::Set(T x/* = 0*/)
{
	Set(x, x, x, x);
}

template <typename T>
void Vector4<T>::Set(T x, T y, T z, T w/* = 0*/)
{
	X() = x;
	Y() = y;
	Z() = z;
	W() = w;
}

template <typename T>
T Vector4<T>::X() const
{
	return data[0];
}

template <typename T>
T Vector4<T>::Y() const
{
	return data[1];
}

template <typename T>
T Vector4<T>::Z() const
{
	return data[2];
}

template <typename T>
T Vector4<T>::W() const
{
	return data[3];
}

template <typename T>
T& Vector4<T>::X()
{
	return data[0];
}

template <typename T>
T& Vector4<T>::Y()
{
	return data[1];
}

template <typename T>
T& Vector4<T>::Z()
{
	return data[2];
}

template <typename T>
T& Vector4<T>::W()
{
	return data[3];
}

template <typename T>
T& Vector4<T>::operator[](size_t inIndex)
{
	Assert(inIndex < 4);
	return data[inIndex];
}

template <typename T>
const T& Vector4<T>::operator[](size_t inIndex) const
{
	Assert(inIndex < 4);
	return data[inIndex];
}

template <typename T>
Vector4<T> Vector4<T>::operator + (T x) const
{
	Vector4<T> ret(*this);

	ret.X() += x;
	ret.Y() += x;
	ret.Z() += x;
	ret.W() += x;

	return ret;
}

template <typename T>
Vector4<T> Vector4<T>::operator + (const Vector4<T>& inOther) const
{
	Vector4<T> ret(*this);

	ret.X() += inOther.X();
	ret.Y() += inOther.Y();
	ret.Z() += inOther.Z();
	ret.W() += inOther.W();

	return ret;
}

template <typename T>
Vector4<T> Vector4<T>::operator - (T x) const
{
	Vector4<T> ret(*this);

	ret.X() -= x;
	ret.Y() -= x;
	ret.Z() -= x;
	ret.W() -= x;

	return ret;
}

template <typename T>
Vector4<T> Vector4<T>::operator - (const Vector4<T>& inOther) const
{
	Vector4<T> ret(*this);

	ret.X() -= inOther.X();
	ret.Y() -= inOther.Y();
	ret.Z() -= inOther.Z();
	ret.W() -= inOther.W();

	return ret;
}

template <typename T>
Vector4<T> Vector4<T>::operator - () const
{
	Vector4<T> ret(*this);

	ret.X() = -ret.X();
	ret.Y() = -ret.Y();
	ret.Z() = -ret.Z();
	ret.W() = -ret.W();

	return ret;
}

template <typename T>
Vector4<T> Vector4<T>::operator * (T x) const
{
	Vector4<T> ret(*this);

	ret.X() *= x;
	ret.Y() *= x;
	ret.Z() *= x;
	ret.W() *= x;

	return ret;
}

template <typename T>
Vector4<T> Vector4<T>::operator / (T x) const
{
	T d = ((T)1) / x;
	return *this * d;
}

template <typename T>
T Vector4<T>::Dot(const Vector4<T>& v0, const Vector4<T>& v1)
{
	//T res = ((T)0);
	//for (int i = 0; i < 4; i++)
	//{
	//	res += v0[i] * v1[i];
	//}
	//return res;

	return (v0.X() * v1.X()) + (v0.Y() * v1.Y()) + (v0.Z() * v1.Z()) + (v0.W() * v0.W());
}

template <typename T>
Vector4<T> Vector4<T>::Cross(const Vector4<T>& v0, const Vector4<T>& v1)
{
	Vector4<T> ret;

	ret.X() = (v0.Y() * v1.Z()) - (v0.Z() * v1.Y());
	ret.Y() = (v0.Z() * v1.X()) - (v0.X() * v1.Z());
	ret.Z() = (v0.X() * v1.Y()) - (v0.Y() * v1.X());

	return ret;
}

template <typename T>
T Vector4<T>::LengthSquared() const
{
	return Dot(*this, *this);
}

template <typename T>
T Vector4<T>::Length() const
{
	return sqrt(LengthSquared());
}

template <typename T>
Vector4<T> Vector4<T>::Normalize(const Vector4<T>& v)
{
	T length = v.Length();
	return v / length;
}