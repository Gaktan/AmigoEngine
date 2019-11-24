#pragma once

// Code between Begin(X) and End(X) is generated. Do not modify it

// BeginInclude
	#include "shaders\generated\PixelShader_PixelShader_PS.generated.h"
	#include "shaders\generated\ShaderFileToTest_Test_10_PS.generated.h"
	#include "shaders\generated\ShaderFileToTest_Test_11_PS.generated.h"
	#include "shaders\generated\ShaderFileToTest_Test_12_PS.generated.h"
	#include "shaders\generated\ShaderFileToTest_Test_14_PS.generated.h"
	#include "shaders\generated\ShaderFileToTest_Test_15_PS.generated.h"
	#include "shaders\generated\ShaderFileToTest_Test_16_PS.generated.h"
	#include "shaders\generated\ShaderFileToTest_Test_19_VS.generated.h"
	#include "shaders\generated\ShaderFileToTest_Test_22_PS.generated.h"
	#include "shaders\generated\ShaderFileToTest_Test_23_PS.generated.h"
	#include "shaders\generated\ShaderFileToTest_Test_24_PS.generated.h"
	#include "shaders\generated\VertexShader_VertexShader_VS.generated.h"
// EndInclude

#define GET_SHADER_BYTECODE(__name) SHADER_BYTECODE_ ## __name
#define INIT_SHADER_BYTECODE(__name) D3D12_SHADER_BYTECODE GET_SHADER_BYTECODE(__name) = { g_ ## __name, sizeof(g_ ## __name) }

// BeginShaderByteCode
	INIT_SHADER_BYTECODE(PixelShader);
	INIT_SHADER_BYTECODE(Test_10);
	INIT_SHADER_BYTECODE(Test_11);
	INIT_SHADER_BYTECODE(Test_12);
	INIT_SHADER_BYTECODE(Test_14);
	INIT_SHADER_BYTECODE(Test_15);
	INIT_SHADER_BYTECODE(Test_16);
	INIT_SHADER_BYTECODE(Test_19);
	INIT_SHADER_BYTECODE(Test_22);
	INIT_SHADER_BYTECODE(Test_23);
	INIT_SHADER_BYTECODE(Test_24);
	INIT_SHADER_BYTECODE(VertexShader);
// EndShaderByteCode
