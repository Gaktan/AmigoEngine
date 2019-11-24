// ShaderCompiler. Name: VertexShader, Type: VS

struct ModelViewProjection
{
    float4x4 MVP;
};

// TODO: Make this a little nicer.
#if SHADER_MODEL > 5
ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);
#else
cbuffer ModelViewProjectionCB : register(b0)
{
	ModelViewProjection ModelViewProjectionCB;
}
#endif

struct VertexPosColor
{
    float4 Position : POSITION;
    float4 Color    : COLOR;
};

struct VertexShaderOutput
{
	float4 Color    : COLOR;
    float4 Position : SV_Position;
};

VertexShaderOutput main(VertexPosColor IN)
{
	VertexShaderOutput OUT;

	OUT.Position = mul(ModelViewProjectionCB.MVP, float4(IN.Position.xyz, 1.0f));
	OUT.Color = float4(IN.Color.rgb, 1.0f);

	return OUT;
}