// Make sure the following fails
/*
ShaderCompiler.                          , EntryPoint: main, Type: PS
ShaderCompiler.                          , EntryPoint: main,         
ShaderCompiler.                          ,                 , Type: PS
ShaderCompiler.                          ,                 ,         
*/

// These are ok though
// ShaderCompiler. Name: Test_10,                 , Type: PS
// ShaderCompiler. Name: Test_11, EntryPoint: main,         
// ShaderCompiler. Name: Test_12,                 ,         

//		 ShaderCompiler.				 Name:		  ShaderFileToTest_14		 ,				 EntryPoint				 :				  main,				 Type		 :			PS			 
//ShaderCompiler.Name:Test_15,EntryPoint:main,Type:PS
// ShaderCompiler. Name: Test_16, EntryPoint: main, Type: PS
// ShaderCompiler. Name: Test_17, EntryPoint: main, Type: PiXeL
// ShaderCompiler. Name: Test_18, EntryPoint: main, Type: fragment
// ShaderCompiler. Name: Test_19, EntryPoint: mainVS, Type: VS
// ShaderCompiler. Name: Test_20, EntryPoint: mainVS, Type: vertex


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

float4 main()   : SV_Target
{
	return 0.0;
}

VertexOutput mainVS(VertexInput IN)
{
	VertexOutput OUT;

	OUT.Position = IN.Position * IN.Color;

	return OUT;
}