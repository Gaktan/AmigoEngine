// I know this file looks disgusting, this is just for testing purposes.

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
// ShaderCompiler. Name: Test_17, EntryPoint: main, Type: PiXeL
// ShaderCompiler. Name: Test_18, EntryPoint: main, Type: fragment
// ShaderCompiler. Name: Test_19, EntryPoint: mainVS, Type: VS
// ShaderCompiler. Name: Test_20, EntryPoint: mainVS, Type: vertex

// ShaderCompiler. Name: Test_22, EntryPoint: mainD, Type: PS, Defines: TEST22
// ShaderCompiler. Name: Test_23, EntryPoint: mainD, Type: PS, Defines: TEST23=1;TEST24=0
// ShaderCompiler. Name: Test_24, EntryPoint: mainD, Type: PS, Defines: TEST23 = 0; TEST24=1


struct X1234
{
    float1 x1; float2 x2;  float3 x3;   float4 x4;
};

struct ModelViewProjection
{
    float4x4 MVP;
};

struct VertexInput
{
    nointerpolation float4 Position : POSITION;
    float4 Color    : COLOR;
};

struct VertexOutput
{
    float4 Position : SV_Position;
};


// Nameless2 no space
struct{nointerpolation float4 Nameless2Position:POSITION;float4 Nameless2Color;}Nameless2;

// Nameless3 weird spaces
struct		              {		    
nointerpolation                              
				float4
Nameless3Position
			:
		 POSITION
	   ;
	  float4 
		Nameless3Color
		;
} Nameless3
;


struct Interpolators
{
	nointerpolation float4 Color    : COLOR;
    float4 Position : SV_Position;
};


struct InsideStruct
{
	X1234 X;
};

#define DEFINED 10

struct Array
{
	float4 _m[4];
	X1234 p   [12];
	nointerpolation float4 Color	 
								[DEFINED]    : COLOR;
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
VertexOutput mainVS(VertexInput IN)
{
	VertexOutput OUT;

	OUT.Position = IN.Position * IN.Color;

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