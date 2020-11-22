#pragma once

struct VertexPosUV
{
	float4 Position	: POSITION;
	float4 UV		: TEXCOORD;
};

struct VertexPosUVNormal
{
	float4 Position	: POSITION;
	float4 UV		: TEXCOORD;
	float4 Normal	: NORMAL;
};
