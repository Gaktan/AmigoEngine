// ShaderCompiler. Name: TextureCopyPS, Type: PS, EntryPoint: MainPS
// ShaderCompiler. Name: TextureCopyVS, Type: VS, EntryPoint: MainVS

Texture2D<float4> SceneTexture : register(t0);
SamplerState Sampler : register(s0);

struct VertexPosUV
{
	float4 Position	: POSITION;
	float4 UV		: TEXCOORD;
};

struct VertexShaderOutput
{
	float2 UV		: TEXCOORD;
    float4 Position : SV_Position;
};

VertexShaderOutput MainVS(VertexPosUV IN)
{
	VertexShaderOutput OUT;

	OUT.Position	= float4(IN.Position.xyz, 1.0f);
	OUT.UV			= IN.UV.xy;

	return OUT;
}

float4 MainPS(VertexShaderOutput IN) : SV_Target
{
    return SceneTexture.SampleLevel(Sampler, IN.UV, 0);
}