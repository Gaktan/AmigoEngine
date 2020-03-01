#pragma once

// nonstandard extension used: nameless struct/union
#pragma warning(disable : 4201)

template <typename T>
class Matrix4
{
protected:
	union
	{
		Vector4<T> r[4];
		float m[4][4];
		struct
		{
			float _m00, _m01, _m02, _m03;
			float _m10, _m11, _m12, _m13;
			float _m20, _m21, _m22, _m23;
			float _m30, _m31, _m32, _m33;
		};
	};

	static Matrix4<T> s_Zero;
	static Matrix4<T> s_Identity;

public:

	Matrix4(bool inIdentity = false)
	{
		if (inIdentity)
		{
			*this = s_Identity;
		}
#if defined(_DEBUG)
		else
		{
			*this = s_Zero;
		}
#endif
	}

	Matrix4(const Matrix4<T>& inOther)
	{
		::memcpy(m, inOther.m, sizeof(T) * 4 * 4);
	}

	Matrix4(const Vector4<T>& inRow0, const Vector4<T>& inRow1, const Vector4<T>& inRow2, const Vector4<T>& inRow3)
	{
		r[0] = inRow0;
		r[1] = inRow1;
		r[2] = inRow2;
		r[3] = inRow3;
	}

	T* operator[](size_t inRow)
	{
		return &r[inRow];
	}

	const T* operator[](size_t inRow) const
	{
		return &r[inRow];
	}

	Matrix4<T> Mul(const Matrix4<T>& inOther) const
	{
		Matrix4<T> ret;

		float x = m[0][0];
		float y = m[0][1];
		float z = m[0][2];
		float w = m[0][3];
		// Perform the operation on the first row
		ret.m[0][0] = (inOther.m[0][0] * x) + (inOther.m[1][0] * y) + (inOther.m[2][0] * z) + (inOther.m[3][0] * w);
		ret.m[0][1] = (inOther.m[0][1] * x) + (inOther.m[1][1] * y) + (inOther.m[2][1] * z) + (inOther.m[3][1] * w);
		ret.m[0][2] = (inOther.m[0][2] * x) + (inOther.m[1][2] * y) + (inOther.m[2][2] * z) + (inOther.m[3][2] * w);
		ret.m[0][3] = (inOther.m[0][3] * x) + (inOther.m[1][3] * y) + (inOther.m[2][3] * z) + (inOther.m[3][3] * w);

		// Repeat for all the inOther rows
		x = m[1][0];
		y = m[1][1];
		z = m[1][2];
		w = m[1][3];
		ret.m[1][0] = (inOther.m[0][0] * x) + (inOther.m[1][0] * y) + (inOther.m[2][0] * z) + (inOther.m[3][0] * w);
		ret.m[1][1] = (inOther.m[0][1] * x) + (inOther.m[1][1] * y) + (inOther.m[2][1] * z) + (inOther.m[3][1] * w);
		ret.m[1][2] = (inOther.m[0][2] * x) + (inOther.m[1][2] * y) + (inOther.m[2][2] * z) + (inOther.m[3][2] * w);
		ret.m[1][3] = (inOther.m[0][3] * x) + (inOther.m[1][3] * y) + (inOther.m[2][3] * z) + (inOther.m[3][3] * w);

		x = m[2][0];
		y = m[2][1];
		z = m[2][2];
		w = m[2][3];
		ret.m[2][0] = (inOther.m[0][0] * x) + (inOther.m[1][0] * y) + (inOther.m[2][0] * z) + (inOther.m[3][0] * w);
		ret.m[2][1] = (inOther.m[0][1] * x) + (inOther.m[1][1] * y) + (inOther.m[2][1] * z) + (inOther.m[3][1] * w);
		ret.m[2][2] = (inOther.m[0][2] * x) + (inOther.m[1][2] * y) + (inOther.m[2][2] * z) + (inOther.m[3][2] * w);
		ret.m[2][3] = (inOther.m[0][3] * x) + (inOther.m[1][3] * y) + (inOther.m[2][3] * z) + (inOther.m[3][3] * w);

		x = m[3][0];
		y = m[3][1];
		z = m[3][2];
		w = m[3][3];
		ret.m[3][0] = (inOther.m[0][0] * x) + (inOther.m[1][0] * y) + (inOther.m[2][0] * z) + (inOther.m[3][0] * w);
		ret.m[3][1] = (inOther.m[0][1] * x) + (inOther.m[1][1] * y) + (inOther.m[2][1] * z) + (inOther.m[3][1] * w);
		ret.m[3][2] = (inOther.m[0][2] * x) + (inOther.m[1][2] * y) + (inOther.m[2][2] * z) + (inOther.m[3][2] * w);
		ret.m[3][3] = (inOther.m[0][3] * x) + (inOther.m[1][3] * y) + (inOther.m[2][3] * z) + (inOther.m[3][3] * w);

		return ret;
	}

	Matrix4<T> Tanslate(const Vector4<T>& inPosition) const
	{
		Matrix4<T> ret(*this);

		ret._m30 += inPosition.x;
		ret._m31 += inPosition.y;
		ret._m32 += inPosition.z;

		return ret;
	}

	static Matrix4<T> CreateRotationMatrix(const Vector4f& inAxis, float inAngle)
	{
		Matrix4<T> ret(true);

		const Vector4f rotationAxis = Vector4f::Normalize(inAxis);

		float s = sin(inAngle);
		float c = cos(inAngle);
		float t = 1.0f - c;

		float x = rotationAxis.x;
		float y = rotationAxis.y;
		float z = rotationAxis.z;

		ret._m00 = t * x*x + c;
		ret._m01 = t * y*x + (s * z);
		ret._m02 = t * z*x - s * y;

		ret._m10 = t * x*y - s * z;
		ret._m11 = t * y*y + c;
		ret._m12 = t * z*y + s * x;

		ret._m20 = t * x*z + s * y;
		ret._m21 = t * y*z - s * x;
		ret._m22 = t * z*z + c;

		return ret;
	}

	static Matrix4<T> CreateLookAtMatrix(const Vector4f& inEye, const Vector4f& inFocus, const Vector4f& inUpVector = { 0, 0, 1 })
	{
		Vector4f forward	= Vector4f::Normalize(inFocus - inEye);
		Vector4f right		= Vector4f::Cross(Vector4f::Normalize(inUpVector), forward);
		Vector4f up			= Vector4f::Cross(forward, right);

		Matrix4<T> ret(true);

		ret._m00 = right.x;
		ret._m01 = right.y;
		ret._m02 = right.z;
		ret._m10 = up.x;
		ret._m11 = up.y;
		ret._m12 = up.z;
		ret._m20 = forward.x;
		ret._m21 = forward.y;
		ret._m22 = forward.z;

		ret._m30 = 0.0f - Vector4f::Dot(right, inEye);
		ret._m31 = 0.0f - Vector4f::Dot(up, inEye);
		ret._m32 = 0.0f - Vector4f::Dot(forward, inEye);

		return ret;
	}

	static Matrix4<T> CreatePerspectiveMatrix(float inFOV, float inAspectRatio, float inZNear, float inZFar)
	{
		inFOV *= 0.5f;

		float cosFov	= cos(inFOV);
		float sinFov	= sin(inFOV);
		float height	= cosFov / sinFov;
		float width		= height / inAspectRatio;
		float range		= inZFar / (inZFar - inZNear);

		Matrix4 Result;

		Result._m00 = width;
		Result._m01 = 0.0f;
		Result._m02 = 0.0f;
		Result._m03 = 0.0f;

		Result._m10 = 0.0f;
		Result._m11 = height;
		Result._m12 = 0.0f;
		Result._m13 = 0.0f;

		Result._m20 = 0.0f;
		Result._m21 = 0.0f;
		Result._m22 = range;
		Result._m23 = 1.0f;

		Result._m30 = 0.0f;
		Result._m31 = 0.0f;
		Result._m32 = -range * inZNear;
		Result._m33 = 0.0f;

		return Result;
	}

	Matrix4<T> Transpose() const
	{
		Matrix4<T> ret;

		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				ret.m[j][i] = this->m[i][j];
			}
		}

		return ret;
	}
};

#pragma warning(default : 4201)

typedef Matrix4<float> Matrix4f;

Matrix4f Matrix4f::s_Zero		= { 0.0f };
Matrix4f Matrix4f::s_Identity	=
{
	{ 1.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 1.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f }
};