// ShaderCompiler. Name: TransparentShader, Type: PS

Texture2D<float4> SceneTexture : register(t0);
SamplerState Sampler : register(s0);

struct PixelShaderInput
{
	float2 UV : TEXCOORD;
};

float4 main(PixelShaderInput IN) : SV_Target
{
	float alpha = 0.3;
    return float4(SceneTexture.SampleLevel(Sampler, IN.UV, 0).rgb, alpha);
}