// ShaderCompiler. Name: DefaultPS, Type: PS, EntryPoint: MainPS
// ShaderCompiler. Name: DefaultVS, Type: VS, EntryPoint: MainVS

#include "VertexLayouts.h"

struct DefaultConstantBuffer
{
	float4x4 MVP;
};

// TODO: Make this a little nicer.
#if SHADER_MODEL > 50
ConstantBuffer<DefaultConstantBuffer> DefaultCB : register(b0);
#else
cbuffer DefaultCB : register(b0)
{
	DefaultConstantBuffer ModelViewProjectionCB;
}
#endif

struct VertexShaderOutput
{
	float2 UV		: TEXCOORD;
    float4 Position : SV_Position;
};

VertexShaderOutput MainVS(VertexPosUVNormal IN)
{
	VertexShaderOutput OUT;

	OUT.Position	= mul(float4(IN.Position.xyz, 1.0f), DefaultCB.MVP);
	OUT.UV			= IN.UV.xy;

	return OUT;
}

Texture2D<float4> SceneTexture : register(t0);
SamplerState Sampler : register(s0);

float4 MainPS(VertexShaderOutput IN) : SV_Target
{
    return SceneTexture.SampleLevel(Sampler, IN.UV, 0);
}