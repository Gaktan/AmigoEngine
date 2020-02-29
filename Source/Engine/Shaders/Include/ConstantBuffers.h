#pragma once

#include "Engine.h"
#include "Math/Vector4.h"

// Dummy Vector types until we get them for real
using Vector2f = float[2];
using Vector3f = float[3];

// Code between Begin(X) and End(X) is generated. Do not modify it

namespace ConstantBuffer
{
// BeginConstantBuffer
struct X1234
{
	float x1;
	Vector2f x2;
	Vector3f x3;
	Vector4f x4;
};

struct ModelViewProjection2
{
	float MVP[4][4];
};

struct MultipleArrays
{
	int32 i3[1][2][3][4][6][7][3];
};

struct InsideStruct
{
	X1234 X;
};

struct ModelViewProjection
{
	float MVP[4][4];
	Vector4f ColorMul;
};
// EndConstantBuffer
}