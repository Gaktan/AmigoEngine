#pragma once

#include "Engine.h"

// TODO: Defines are tricky because we souldn't allow to generate the same structure with different define values
// This case is fine because it's a constant, but think of a define from a Shader Header.
#define SEVEN 7

// Code between Begin(X) and End(X) is generated. Do not modify it

namespace ConstantBuffers
{
// BeginConstantBuffer
struct DefaultConstantBuffer
{
	Mat4x4 MVP;
};

struct TestX1234
{
	float x1;
	Vec2 x2;
	Vec3 x3;
	Vec4 x4;
};

struct TestModelViewProjection2
{
	Mat4x4 MVP;
};

struct TestMultipleArrays
{
	int32 i3[1][2] [3]	[4 ] [ 6 ]	[SEVEN][3];
	Mat4x4 x4[1] [2* SEVEN-1];
};

struct TestInsideStruct
{
	TestX1234 X;
};
// EndConstantBuffer
}