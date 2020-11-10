// ShaderCompiler. Name: DefaultPS, Type: PS, EntryPoint: MainPS
// ShaderCompiler. Name: DefaultVS, Type: VS, EntryPoint: MainVS

struct ModelViewProjection
{
    float4x4	Model;
	float4x4	View;
	float4x4	Projection;
};

// TODO: Make this a little nicer.
#if SHADER_MODEL > 50
ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);
#else
cbuffer ModelViewProjectionCB : register(b0)
{
	ModelViewProjection ModelViewProjectionCB;
}
#endif

struct VertexPosUVNormal
{
	float4 Position	: POSITION;
	float4 UV		: TEXCOORD;
	float4 Normal	: NORMAL;
};

struct VertexShaderOutput
{
	float2 UV		: TEXCOORD;
    float4 Position : SV_Position;
};

VertexShaderOutput MainVS(VertexPosUVNormal IN)
{
	VertexShaderOutput OUT;

	OUT.Position	= mul(ModelViewProjectionCB.Projection, mul(ModelViewProjectionCB.View, mul(ModelViewProjectionCB.Model, float4(IN.Position.xyz, 1.0f))));
	OUT.UV			= IN.UV.xy;

	return OUT;
}

Texture2D<float4> SceneTexture : register(t0);
SamplerState Sampler : register(s0);

struct PixelShaderInput
{
	float2 UV : TEXCOORD;
};

float4 MainPS(PixelShaderInput IN) : SV_Target
{
    return SceneTexture.SampleLevel(Sampler, IN.UV, 0);
}