// ShaderCompiler. Name: TransparentShader, Type: PS

Texture2D<float4> SceneTexture : register(t0);
SamplerState Sampler : register(s0);

struct VertexShaderOutput
{
	float2 UV		: TEXCOORD;
    float4 Position : SV_Position;
};

float4 main(VertexShaderOutput IN) : SV_Target
{
	float alpha = 0.3;
    return float4(SceneTexture.SampleLevel(Sampler, IN.UV, 0).rgb, alpha);
}