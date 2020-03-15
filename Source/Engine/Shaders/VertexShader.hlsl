// ShaderCompiler. Name: VertexShader, Type: VS

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

VertexShaderOutput main(VertexPosUVNormal IN)
{
	VertexShaderOutput OUT;

	float4x4 mvp_matrix = ModelViewProjectionCB.Projection * ModelViewProjectionCB.View * ModelViewProjectionCB.Model;
	float4 res = mul(ModelViewProjectionCB.Projection, mul(ModelViewProjectionCB.View, mul(ModelViewProjectionCB.Model, float4(IN.Position.xyz, 1.0f))));

	OUT.Position	= mul(mvp_matrix, float4(IN.Position.xyz, 1.0f));
	OUT.Position	= res;
	OUT.UV			= IN.UV.xy;

	return OUT;
}