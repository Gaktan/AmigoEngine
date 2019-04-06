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

	static Matrix4<T> ZERO;
	static Matrix4<T> IDENTITY;

public:

	Matrix4(bool identity = false)
	{
		if (identity)
		{
			*this = IDENTITY;
		}
#if defined(_DEBUG)
		else
		{
			*this = ZERO;
		}
#endif
	}

	Matrix4(const Matrix4<T>& other)
	{
		memcpy(m, other.m, sizeof(T) * 4 * 4);
	}

	Matrix4(const Vector4<T>& row0, const Vector4<T>& row1, const Vector4<T>& row2, const Vector4<T>& row3)
	{
		r[0] = row0;
		r[1] = row1;
		r[2] = row2;
		r[3] = row3;
	}

	T* operator[](size_t row)
	{
		return &r[row];
	}

	const T* operator[](size_t row) const
	{
		return &r[row];
	}

	Matrix4<T> Mul(const Matrix4<T>& other) const
	{
		Matrix4<T> ret;

		float x = m[0][0];
		float y = m[0][1];
		float z = m[0][2];
		float w = m[0][3];
		// Perform the operation on the first row
		ret.m[0][0] = (other.m[0][0] * x) + (other.m[1][0] * y) + (other.m[2][0] * z) + (other.m[3][0] * w);
		ret.m[0][1] = (other.m[0][1] * x) + (other.m[1][1] * y) + (other.m[2][1] * z) + (other.m[3][1] * w);
		ret.m[0][2] = (other.m[0][2] * x) + (other.m[1][2] * y) + (other.m[2][2] * z) + (other.m[3][2] * w);
		ret.m[0][3] = (other.m[0][3] * x) + (other.m[1][3] * y) + (other.m[2][3] * z) + (other.m[3][3] * w);
		
		// Repeat for all the other rows
		x = m[1][0];
		y = m[1][1];
		z = m[1][2];
		w = m[1][3];
		ret.m[1][0] = (other.m[0][0] * x) + (other.m[1][0] * y) + (other.m[2][0] * z) + (other.m[3][0] * w);
		ret.m[1][1] = (other.m[0][1] * x) + (other.m[1][1] * y) + (other.m[2][1] * z) + (other.m[3][1] * w);
		ret.m[1][2] = (other.m[0][2] * x) + (other.m[1][2] * y) + (other.m[2][2] * z) + (other.m[3][2] * w);
		ret.m[1][3] = (other.m[0][3] * x) + (other.m[1][3] * y) + (other.m[2][3] * z) + (other.m[3][3] * w);
		
		x = m[2][0];
		y = m[2][1];
		z = m[2][2];
		w = m[2][3];
		ret.m[2][0] = (other.m[0][0] * x) + (other.m[1][0] * y) + (other.m[2][0] * z) + (other.m[3][0] * w);
		ret.m[2][1] = (other.m[0][1] * x) + (other.m[1][1] * y) + (other.m[2][1] * z) + (other.m[3][1] * w);
		ret.m[2][2] = (other.m[0][2] * x) + (other.m[1][2] * y) + (other.m[2][2] * z) + (other.m[3][2] * w);
		ret.m[2][3] = (other.m[0][3] * x) + (other.m[1][3] * y) + (other.m[2][3] * z) + (other.m[3][3] * w);
		
		x = m[3][0];
		y = m[3][1];
		z = m[3][2];
		w = m[3][3];
		ret.m[3][0] = (other.m[0][0] * x) + (other.m[1][0] * y) + (other.m[2][0] * z) + (other.m[3][0] * w);
		ret.m[3][1] = (other.m[0][1] * x) + (other.m[1][1] * y) + (other.m[2][1] * z) + (other.m[3][1] * w);
		ret.m[3][2] = (other.m[0][2] * x) + (other.m[1][2] * y) + (other.m[2][2] * z) + (other.m[3][2] * w);
		ret.m[3][3] = (other.m[0][3] * x) + (other.m[1][3] * y) + (other.m[2][3] * z) + (other.m[3][3] * w);

		return ret;
	}

	static Matrix4<T> CreateRotationMatrix(const Vector4f& axis, float angle)
	{
		Matrix4<T> ret(true);

		const Vector4f rotationAxis = Vector4f::Normalize(axis);

		float s = sin(angle);
		float c = cos(angle);
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
		ret._m22 = t*z*z + c;

		return ret;
	}

	static Matrix4<T> CreateLookAtMatrix(const Vector4f& eye, const Vector4f& focus, const Vector4f& upVector = { 0, 0, 1 })
	{
		Vector4f forward = Vector4f::Normalize(focus - eye);
		Vector4f right = Vector4f::Cross(Vector4f::Normalize(upVector), forward);
		Vector4f up = Vector4f::Cross(forward, right);

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

		ret._m30 = 0.0f - Vector4f::Dot(right, eye);
		ret._m31 = 0.0f - Vector4f::Dot(up, eye);
		ret._m32 = 0.0f - Vector4f::Dot(forward, eye);

		return ret;
	}

	static Matrix4<T> CreatePerspectiveMatrix(float fov, float aspectRatio, float zNear, float zFar)
	{
		fov *= 0.5f;

		float cosFov = cos(fov);
		float sinFov = sin(fov);
		float height = cosFov / sinFov;
		float width = height / aspectRatio;
		float range = zFar / (zFar - zNear);

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
		Result._m32 = -range * zNear;
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

Matrix4f Matrix4f::ZERO = { 0.0f };
Matrix4f Matrix4f::IDENTITY =
{
	{ 1.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 1.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f }
};