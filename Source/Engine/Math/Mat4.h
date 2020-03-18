#pragma once

#include "Math/Math.h"
#include "Math/Vec4.h"

// nonstandard extension used: nameless struct/union
#pragma warning(disable : 4201)

class Mat4
{
protected:
	union
	{
		Vec4 r[4];
		float m[4][4];
		struct
		{
			float _m00, _m01, _m02, _m03;
			float _m10, _m11, _m12, _m13;
			float _m20, _m21, _m22, _m23;
			float _m30, _m31, _m32, _m33;
		};
	};

	static Mat4 s_Zero;
	static Mat4 s_Identity;

public:

	Mat4(bool inIdentity = false)
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

	Mat4(const Mat4& inOther)
	{
		::memcpy(m, inOther.m, sizeof(float) * 4 * 4);
	}

	Mat4(const Vec4& inRow0, const Vec4& inRow1, const Vec4& inRow2, const Vec4& inRow3)
	{
		r[0] = inRow0;
		r[1] = inRow1;
		r[2] = inRow2;
		r[3] = inRow3;
	}

	Vec4* operator[](size_t inRow)
	{
		return &r[inRow];
	}

	const Vec4* operator[](size_t inRow) const
	{
		return &r[inRow];
	}

	Mat4 Mul(const Mat4& inOther) const
	{
		Mat4 ret;

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

	void SetPosition(const Vec4& inPosition)
	{
		r[3] = inPosition;
	}

	Vec4 GetPosition() const
	{
		return r[3];
	}

	static Mat4 CreateRotationMatrix(const Vec4& inAxis, float inAngle)
	{
		Mat4 ret(true);

		const Vec4 rotationAxis = Vec4::Normalize(inAxis);

		float s = sin(inAngle);
		float c = cos(inAngle);
		float t = 1.0f - c;

		float x = rotationAxis.X();
		float y = rotationAxis.Y();
		float z = rotationAxis.Z();

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

	static Mat4 CreateLookAtMatrix(const Vec4& inEye, const Vec4& inFocus, const Vec4& inUpVector = { 0, 1, 0 })
	{
		Vec4 forward	= Vec4::Normalize(inFocus - inEye);
		Vec4 right		= Vec4::Normalize(Vec4::Cross(Vec4::Normalize(inUpVector), forward));
		Vec4 up			= Vec4::Cross(forward, right);

		Mat4 ret(true);

		ret._m00 = right.X();
		ret._m10 = right.Y();
		ret._m20 = right.Z();
		ret._m01 = up.X();
		ret._m11 = up.Y();
		ret._m21 = up.Z();
		ret._m02 = forward.X();
		ret._m12 = forward.Y();
		ret._m22 = forward.Z();

		ret._m30 = -Vec4::Dot(right, inEye);
		ret._m31 = -Vec4::Dot(up, inEye);
		ret._m32 = -Vec4::Dot(forward, inEye);

		return ret;
	}

	static Mat4 CreatePerspectiveMatrix(float inFOV, float inAspectRatio, float inZNear, float inZFar)
	{
		inFOV *= 0.5f;

		float cosFov	= Math::Cos(inFOV);
		float sinFov	= Math::Sin(inFOV);
		float height	= cosFov / sinFov;
		float width		= height / inAspectRatio;
		float range		= inZFar / (inZFar - inZNear);

		Mat4 Result;

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

	Mat4 Transposed() const
	{
		Mat4 ret;

		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				ret.m[j][i] = this->m[i][j];
			}
		}

		return ret;
	}

	Vec4 GetForward() const
	{
		Vec4 forward(_m02, _m12, _m22, _m32);
		return forward;
	}

	Vec4 GetRight() const
	{
		Vec4 right(_m00, _m10, _m20, _m30);
		return right;
	}

	Vec4 GetUp() const
	{
		Vec4 up(_m01, _m11, _m21, _m31);
		return up;
	}
};

#pragma warning(default : 4201)

Mat4 Mat4::s_Zero		= 
{
	{ 0.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 0.0f }
};

Mat4 Mat4::s_Identity	=
{
	{ 1.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 1.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f }
};