#pragma once

#include "Engine.h"
#include "Math/Math.h"

// Need to include ConstantBuffers because some structs needed here are declarer there
#include "Shaders/Include/ConstantBuffers.h"
using namespace ConstantBuffers;

// TODO: Defines are tricky because we souldn't allow to generate the same structure with different define values
// This case is fine because it's a constant, but think of a define from a Shader Header.
#define DEFINED 10

// Code between Begin(X) and End(X) is generated. Do not modify it

namespace VertexFormats
{
// BeginVertexFormat
struct VertexPosUV
{
	Vec3 Position;
	Vec2 UV;
};

struct VertexPosUVNormal
{
	Vec3 Position;
	Vec2 UV;
	Vec3 Normal;
};

struct TestVertexInput
{
	Vec4 Position;
	Vec4 Color;
};

struct TestArray
{
	Vec4 _m[4];
	TestX1234 p[12];
	Vec4 Color[DEFINED];
};
// EndVertexFormat
}

namespace VertexInputLayouts
{
// BeginVertexInputLayout
static D3D12_INPUT_ELEMENT_DESC VertexPosUV[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

static D3D12_INPUT_ELEMENT_DESC VertexPosUVNormal[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

static D3D12_INPUT_ELEMENT_DESC TestVertexInput[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

static D3D12_INPUT_ELEMENT_DESC TestArray[] =
{
	{ "", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "", 0, DXGI_FORMAT_R32_TYPELESS, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};
// EndVertexInputLayout
}