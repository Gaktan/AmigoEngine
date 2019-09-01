#pragma once

// nonstandard extension used: nameless struct/union
#pragma warning(disable : 4201)

template <typename T>
class Vector4
{
protected:
	union
	{
		T data[4];
		struct
		{
			T x;
			T y;
			T z;
			T w;
		};
	};

public:
	Vector4(T x = 0) :
		x(x),
		y(x),
		z(x),
		w(x)
	{
	}

	Vector4(T _x, T _y, T _z, T _w = 0) :
		x(_x),
		y(_y),
		z(_z),
		w(_w)
	{
	}

	Vector4(const Vector4<T>& inOther) :
		x(inOther.x),
		y(inOther.y),
		z(inOther.z),
		w(inOther.w)
	{
	}

	void Set(T x = 0)
	{
		memset(data, x, sizeof(T) * 4);
	}

	void Set(T _x, T _y, T _z, T _w = 0)
	{
		x = _x;
		y = _y;
		z = _z;
		w = _w;
	}


	T& operator[](size_t inIndex)
	{
		//assert(index < 4);
		return data[inIndex];
	}

	const T& operator[](size_t inIndex) const
	{
		//assert(index < 4);
		return data[inIndex];
	}

	Vector4<T> operator + (T X) const
	{
		Vector4<T> ret(*this);

		ret.x += X;
		ret.y += X;
		ret.z += X;
		ret.w += X;

		return ret;
	}

	Vector4<T> operator + (const Vector4<T>& inOther) const
	{
		Vector4<T> ret(*this);

		ret.x += inOther.x;
		ret.y += inOther.y;
		ret.z += inOther.z;
		ret.w += inOther.w;

		return ret;
	}

	Vector4<T> operator - (T X) const
	{
		Vector4<T> ret(*this);

		ret.x -= X;
		ret.y -= X;
		ret.z -= X;
		ret.w -= X;

		return ret;
	}

	Vector4<T> operator - (const Vector4<T>& inOther) const
	{
		Vector4<T> ret(*this);

		ret.x -= inOther.x;
		ret.y -= inOther.y;
		ret.z -= inOther.z;
		ret.w -= inOther.w;

		return ret;
	}

	Vector4<T> operator * (T X) const
	{
		Vector4<T> ret(*this);

		ret.x *= X;
		ret.y *= X;
		ret.z *= X;
		ret.w *= X;

		return ret;
	}

	Vector4<T> operator / (T X) const
	{
		float d = 1 / X;
		return *this * d;
	}

	static float Dot(const Vector4<T>& v0, const Vector4<T>& v1)
	{
		return (v0.x * v1.x) + (v0.y * v1.y) + (v0.z * v1.z) + (v0.w * v0.w);
	}

	static Vector4<T> Cross(const Vector4<T>& v0, const Vector4<T>& v1)
	{
		Vector4<T> ret;

		ret.x = (v0.y * v1.z) - (v0.z * v1.y);
		ret.y = (v0.z * v1.x) - (v0.x * v1.z);
		ret.z = (v0.x * v1.y) - (v0.y * v1.x);

		return ret;
	}

	float LengthSquared() const
	{
		return Dot(*this, *this);
	}

	float Length() const
	{
		return sqrt(LengthSquared());
	}

	static Vector4<T> Normalize(const Vector4<T>& v)
	{
		float length = v.Length();
		return v / length;
	}
};

#pragma warning(default : 4201)

typedef Vector4<float> Vector4f;