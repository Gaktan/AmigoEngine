#pragma once

struct VertexPosUV
{
	float3 Position	: POSITION;
	float2 UV		: TEXCOORD;
};

struct VertexPosUVNormal
{
	float3 Position	: POSITION;
	float2 UV		: TEXCOORD;
	float3 Normal	: NORMAL;
};
