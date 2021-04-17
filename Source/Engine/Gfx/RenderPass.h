#pragma once

#include "DX12/DX12Includes.h"

class DrawableObject;

enum RenderPass : uint32
{
	OpaqueGeometry = 0,
	Transparent,
	Count
};

using RenderBucket	= std::vector<DrawableObject*>;
using RenderBuckets	= RenderBucket[RenderPass::Count];

class RenderPassDesc
{
public:
	static void SetupRenderPassDesc(RenderPass inRenderPass, D3D12_GRAPHICS_PIPELINE_STATE_DESC& outDesc);
};