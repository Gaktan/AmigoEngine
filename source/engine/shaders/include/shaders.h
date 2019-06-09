#pragma once

// BeginInclude
	#include "shaders\generated\PixelShader_PixelShader_ps.generated.h"
	#include "shaders\generated\ShaderFileToTest_Test_10_ps.generated.h"
	#include "shaders\generated\ShaderFileToTest_Test_11_ps.generated.h"
	#include "shaders\generated\ShaderFileToTest_Test_12_ps.generated.h"
	#include "shaders\generated\ShaderFileToTest_Test_14_ps.generated.h"
	#include "shaders\generated\ShaderFileToTest_Test_15_ps.generated.h"
	#include "shaders\generated\ShaderFileToTest_Test_16_ps.generated.h"
	#include "shaders\generated\ShaderFileToTest_Test_17_ps.generated.h"
	#include "shaders\generated\ShaderFileToTest_Test_18_ps.generated.h"
	#include "shaders\generated\ShaderFileToTest_Test_19_vs.generated.h"
	#include "shaders\generated\ShaderFileToTest_Test_20_vs.generated.h"
	#include "shaders\generated\VertexShader_VertexShader_vs.generated.h"
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
	INIT_SHADER_BYTECODE(Test_17);
	INIT_SHADER_BYTECODE(Test_18);
	INIT_SHADER_BYTECODE(Test_19);
	INIT_SHADER_BYTECODE(Test_20);
	INIT_SHADER_BYTECODE(VertexShader);
// EndShaderByteCode