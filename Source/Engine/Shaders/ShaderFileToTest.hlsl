// I know this file looks disgusting, this is just for testing purposes. 
// And to make sure we handle white spaces properly

// Make sure the following fails (No shader name)
/*
ShaderCompiler.                          , EntryPoint: main, Type: PS
ShaderCompiler.                          , EntryPoint: main,         
ShaderCompiler.                          ,                 , Type: PS
ShaderCompiler.                          ,                 ,         
ShaderCompiler. Name: Test_21, EntryPoint: mainD, Type: PS
*/

// These are ok though (Type, Entry point, and Defines not specified)
// ShaderCompiler. Name: Test_10,                 , Type: PS
// ShaderCompiler. Name: Test_11, EntryPoint: main,         
// ShaderCompiler. Name: Test_12,                 ,         

//		 ShaderCompiler.				 Name:		  Test_14		 ,				 EntryPoint				 :				  main,				 Type		 :			PS			 
//ShaderCompiler.Name:Test_15,EntryPoint:main,Type:PS
// ShaderCompiler. Name: Test_16, EntryPoint: main, Type: PS
// ShaderCompiler. Name: Test_19, EntryPoint: mainVS, Type: VS

// ShaderCompiler. Name: Test_22, EntryPoint: mainD, Type: PS, Defines: TEST22
// ShaderCompiler. Name: Test_23, EntryPoint: mainD, Type: PS, Defines: TEST23=1;TEST24=0
// ShaderCompiler. Name: Test_24, EntryPoint: mainD, Type: PS, Defines: TEST23 = 0; TEST24=1

#include "VertexLayouts.h"

struct TestX1234
{
    float1 x1; float2 x2;  float3 x3;   float4 x4;
};

struct TestModelViewProjection2
{
    float4x4 MVP;
};

#define SEVEN 7

struct TestMultipleArrays
{
	int3 i3[1][2] [3]	[4 ] [ 6 ]	[SEVEN];
	float4x4 x4		[1] [2* SEVEN-1];
};

// Make sure shader model is defined properly
#if SHADER_MODEL > 50
ConstantBuffer<TestModelViewProjection2> ModelViewProjectionCB : register(b0);
#else
cbuffer name_doesnt_matter : register(b0)
{
	TestModelViewProjection2 ModelViewProjectionCB;
}
#endif

struct TestVertexInput
{
    nointerpolation float4 Position : POSITION;
    float4 Color    : COLOR;
};

struct TestVertexOutput
{
    float4 Position : SV_Position;
};


struct TestInterpolators
{
	nointerpolation float4 Color    : COLOR;
    float4 Position : SV_Position;
};


struct TestInsideStruct
{
	TestX1234 X;
};

#define DEFINED 10

struct TestArray
{
	float4 _m[4];
	TestX1234 p   [12];
	nointerpolation float4 Color	 
								[DEFINED]    : COLOR			
		;
};

// Make sure PIXEL_SHADER is properly defined
#ifdef PIXEL_SHADER
float4 main()   : SV_Target
{
	return 0.0;
}
#endif

// Make sure VERTEX_SHADER is properly defined
#ifdef VERTEX_SHADER
TestVertexOutput mainVS(TestVertexInput IN)
{
	TestVertexOutput OUT;

	// This is bullshit code just to make sure it compiles
	OUT.Position = mul(ModelViewProjectionCB.MVP, IN.Position * IN.Color);

	return OUT;
}
#endif

#ifdef TEST22
float4 TestingDefines()
{
	return 22.0;
}
#else //TEST22
float4 TestingDefines()
{
#if TEST23
	return 23.0;
#elif TEST24
	return 24.0;
#endif
}
#endif// TEST22

// Make sure PIXEL_SHADER is properly defined
#ifdef PIXEL_SHADER
float4 mainD() : SV_Target
{
	return TestingDefines();
}
#endif