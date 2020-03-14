// ShaderCompiler. Name: PixelShader, Type: PS

Texture2D<float4> SceneTexture : register(t0);
SamplerState Sampler : register(s0);

struct PixelShaderInput
{
	float4 Color : COLOR;
	float2 UV		: TEXCOORD;
};

float4 main(PixelShaderInput IN) : SV_Target
{
    return IN.Color * SceneTexture.SampleLevel(Sampler, IN.UV, 0);
}