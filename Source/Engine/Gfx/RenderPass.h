#pragma once

#include "DX12/DX12Includes.h"

class DrawableObject;

enum RenderPass : unsigned int
{
	Geometry = 0,
	Transparent,
	Count
};

typedef std::vector<DrawableObject*> RenderBucket;
typedef RenderBucket RenderBuckets[RenderPass::Count];

class RenderPassDesc
{
public:
	static void SetupRenderPassDesc(RenderPass inRenderPass, D3D12_GRAPHICS_PIPELINE_STATE_DESC& outDesc);
};