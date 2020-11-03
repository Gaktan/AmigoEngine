#include "Engine.h"
#include "RenderPass.h"

void RenderPassDesc::SetupRenderPassDesc(RenderPass inRenderPass, D3D12_GRAPHICS_PIPELINE_STATE_DESC& outDesc)
{
	const D3D12_RENDER_TARGET_BLEND_DESC default_render_target_blend_desc =
	{
		false, false,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
	};

	switch (inRenderPass)
	{
	case RenderPass::Geometry:
	{
		// Setup blend states
		{
			outDesc.BlendState.AlphaToCoverageEnable	= false;
			outDesc.BlendState.IndependentBlendEnable	= false;

			for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
				outDesc.BlendState.RenderTarget[i] = default_render_target_blend_desc;
		}

		// Setup rasterizer states
		{
			outDesc.RasterizerState.FillMode				= D3D12_FILL_MODE_SOLID;
			outDesc.RasterizerState.CullMode				= D3D12_CULL_MODE_BACK;
			outDesc.RasterizerState.FrontCounterClockwise	= false;
			outDesc.RasterizerState.DepthBias				= D3D12_DEFAULT_DEPTH_BIAS;
			outDesc.RasterizerState.DepthBiasClamp			= D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			outDesc.RasterizerState.SlopeScaledDepthBias	= D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			outDesc.RasterizerState.DepthClipEnable			= true;
			outDesc.RasterizerState.MultisampleEnable		= false;
			outDesc.RasterizerState.AntialiasedLineEnable	= true;
			outDesc.RasterizerState.ForcedSampleCount		= 0;
			outDesc.RasterizerState.ConservativeRaster		= D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		}

		// Setup depth stencil states
		{
			const D3D12_DEPTH_STENCILOP_DESC default_stencil_op = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };

			outDesc.DepthStencilState.DepthEnable		= true;
			outDesc.DepthStencilState.DepthWriteMask	= D3D12_DEPTH_WRITE_MASK_ALL;
			outDesc.DepthStencilState.DepthFunc			= D3D12_COMPARISON_FUNC_LESS;
			outDesc.DepthStencilState.StencilEnable		= false;
			outDesc.DepthStencilState.StencilReadMask	= D3D12_DEFAULT_STENCIL_READ_MASK;
			outDesc.DepthStencilState.StencilWriteMask	= D3D12_DEFAULT_STENCIL_WRITE_MASK;

			outDesc.DepthStencilState.FrontFace			= default_stencil_op;
			outDesc.DepthStencilState.BackFace			= default_stencil_op;
		}

		// Only one rendertarget for now
		outDesc.NumRenderTargets = 1;

		// TODO: Grab appropriate format
		constexpr DXGI_FORMAT color_format = DXGI_FORMAT_R8G8B8A8_UNORM;
		constexpr DXGI_FORMAT depth_format = DXGI_FORMAT_D24_UNORM_S8_UINT;

		::memset(&outDesc.RTVFormats[0], DXGI_FORMAT_UNKNOWN, sizeof(outDesc.RTVFormats));
		outDesc.RTVFormats[0]	= color_format;
		outDesc.DSVFormat		= depth_format;

		// Sample desc
		outDesc.SampleDesc.Count	= 1;
		outDesc.SampleDesc.Quality	= 0;
	}
	break;
	case RenderPass::Transparent:
	{
		// Setup blend states
		{
			outDesc.BlendState.AlphaToCoverageEnable	= false;
			outDesc.BlendState.IndependentBlendEnable	= false;

			for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
				outDesc.BlendState.RenderTarget[i] = default_render_target_blend_desc;

			// Enable alpha blending operation for RT0
			outDesc.BlendState.RenderTarget[0] =
			{
				true, false,
				D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
				D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
				D3D12_LOGIC_OP_NOOP,
				D3D12_COLOR_WRITE_ENABLE_ALL,
			};
		}

		// Setup rasterizer states
		{
			outDesc.RasterizerState.FillMode				= D3D12_FILL_MODE_SOLID;
			outDesc.RasterizerState.CullMode				= D3D12_CULL_MODE_BACK;
			outDesc.RasterizerState.FrontCounterClockwise	= false;
			outDesc.RasterizerState.DepthBias				= D3D12_DEFAULT_DEPTH_BIAS;
			outDesc.RasterizerState.DepthBiasClamp			= D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			outDesc.RasterizerState.SlopeScaledDepthBias	= D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			outDesc.RasterizerState.DepthClipEnable			= true;
			outDesc.RasterizerState.MultisampleEnable		= false;
			outDesc.RasterizerState.AntialiasedLineEnable	= true;
			outDesc.RasterizerState.ForcedSampleCount		= 0;
			outDesc.RasterizerState.ConservativeRaster		= D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		}

		// Setup depth stencil states
		{
			const D3D12_DEPTH_STENCILOP_DESC default_stencil_op = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };

			outDesc.DepthStencilState.DepthEnable		= false;
			outDesc.DepthStencilState.DepthWriteMask	= D3D12_DEPTH_WRITE_MASK_ZERO;
			outDesc.DepthStencilState.DepthFunc			= D3D12_COMPARISON_FUNC_LESS;
			outDesc.DepthStencilState.StencilEnable		= false;
			outDesc.DepthStencilState.StencilReadMask	= D3D12_DEFAULT_STENCIL_READ_MASK;
			outDesc.DepthStencilState.StencilWriteMask	= D3D12_DEFAULT_STENCIL_WRITE_MASK;

			outDesc.DepthStencilState.FrontFace			= default_stencil_op;
			outDesc.DepthStencilState.BackFace			= default_stencil_op;
		}

		// Only one rendertarget for now
		outDesc.NumRenderTargets = 1;

		// TODO: Grab appropriate format
		constexpr DXGI_FORMAT color_format = DXGI_FORMAT_R8G8B8A8_UNORM;
		constexpr DXGI_FORMAT depth_format = DXGI_FORMAT_D24_UNORM_S8_UINT;

		::memset(&outDesc.RTVFormats[0], DXGI_FORMAT_UNKNOWN, sizeof(outDesc.RTVFormats));
		outDesc.RTVFormats[0]	= color_format;
		outDesc.DSVFormat		= depth_format;

		// Sample desc
		outDesc.SampleDesc.Count	= 1;
		outDesc.SampleDesc.Quality	= 0;
	}
	break;
	default:
		Assert(false, "RenderPass nor supported.");
	}

}
