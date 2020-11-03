#pragma once

// Code between Begin(X) and End(X) is generated. Do not modify it

// BeginInclude
	#include "Shaders\generated\PixelShader_PixelShader_PS.generated.h"
	#include "Shaders\generated\ShaderFileToTest_Test_10_PS.generated.h"
	#include "Shaders\generated\ShaderFileToTest_Test_11_PS.generated.h"
	#include "Shaders\generated\ShaderFileToTest_Test_12_PS.generated.h"
	#include "Shaders\generated\ShaderFileToTest_Test_14_PS.generated.h"
	#include "Shaders\generated\ShaderFileToTest_Test_15_PS.generated.h"
	#include "Shaders\generated\ShaderFileToTest_Test_16_PS.generated.h"
	#include "Shaders\generated\ShaderFileToTest_Test_19_VS.generated.h"
	#include "Shaders\generated\ShaderFileToTest_Test_22_PS.generated.h"
	#include "Shaders\generated\ShaderFileToTest_Test_23_PS.generated.h"
	#include "Shaders\generated\ShaderFileToTest_Test_24_PS.generated.h"
	#include "Shaders\generated\TransparentShader_TransparentShader_PS.generated.h"
	#include "Shaders\generated\VertexShader_VertexShader_VS.generated.h"
// EndInclude

#define INIT_SHADER_BYTECODE(__name) static const D3D12_SHADER_BYTECODE __name = { g_ ## __name, sizeof(g_ ## __name) }

namespace InlineShaders
{
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
	INIT_SHADER_BYTECODE(TransparentShader);
	INIT_SHADER_BYTECODE(VertexShader);
// EndShaderByteCode
};
