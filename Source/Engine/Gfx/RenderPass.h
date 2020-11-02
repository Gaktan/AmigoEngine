#pragma once

#include "DX12/DX12Includes.h"

enum RenderPass : int
{
	Geometry,
	Transparent,
	Present
};

class RenderPassDesc
{
public:
	static void SetupRenderPassDesc(RenderPass inRenderPass, D3D12_GRAPHICS_PIPELINE_STATE_DESC& outDesc);
};