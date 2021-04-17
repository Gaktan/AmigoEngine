#pragma once

#include "DX12/DX12Includes.h"

class Mesh;
class ShaderObject;
class DX12Resource;

class DrawUtils final
{
private:
	static Mesh						s_FullScreenTriangle;
	// TODO: turn this into a ShaderObject
	static ID3D12PipelineState*		s_PipelineState;
	static ID3D12RootSignature*		s_RootSignature;

public:
	static void Init(ID3D12GraphicsCommandList2* inCommandList);
	static void Destroy();

	static void DrawFullScreenTriangle(ID3D12GraphicsCommandList2* inCommandList, DX12Resource* inTexture);
};