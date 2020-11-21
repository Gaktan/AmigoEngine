#include "Engine.h"
#include "DrawUtils.h"

#include "DX12/DX12CommandQueue.h"
#include "DX12/DX12Device.h"
#include "DX12/DX12RenderTarget.h"

#include "Gfx/Mesh.h"

#include "Shaders/Include/Shaders.h"

Mesh					DrawUtils::s_FullScreenTriangle;
ID3D12PipelineState*	DrawUtils::m_PipelineState = nullptr;
ID3D12RootSignature*	DrawUtils::m_RootSignature = nullptr;

// TODO: This is all a VERY big mess right now. This is temporary, just so I finally can render something on screen
void DrawUtils::Init(ID3D12GraphicsCommandList2* inCommandList)
{
	// Setup meshes
	{
		// TODO: Yuck
		struct VertexPosUV
		{
			Vec4 Position;
			Vec4 UV;
		};

		float vertex_data[] =
		{
			// Position					// UV
			-1.0f, 1.0f, 0.0f, 1.0f,	0.0f, 0.0f, 0.0f, 0.0f,
			3.0f, 1.0f, 0.0f, 1.0f,		2.0f, 0.0f, 0.0f, 0.0f,
			-1.0f, -3.0f, 0.0f, 1.0f,	0.0f, 2.0f, 0.0f, 0.0f,
		};

		// TODO: shouldn't need an index buffer for this, but this is required
		uint16 index_data[] =
		{
			0, 1, 2
		};

		s_FullScreenTriangle.Init(inCommandList,
								  D3D_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
								  &vertex_data[0], 3 * sizeof(VertexPosUV), sizeof(VertexPosUV),
								  &index_data[0], 3 * sizeof(uint16));
	}

	// Setup shaders
	{
		//void ShaderObject::CreateRootSignature()
		{
			// TODO: Root signatures should be associated with the shader or shader bindings instead!

			// Create a root signature.
			D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};
			feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
			if (FAILED(g_RenderingDevice.GetD3DDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
				feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;

			// Allow input layout and deny unnecessary access to certain pipeline stages.
			D3D12_ROOT_SIGNATURE_FLAGS root_signature_flags =
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

			// Create a descriptor table with one entry in our descriptor heap
			CD3DX12_DESCRIPTOR_RANGE1 range { D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 };

			// A single Constant Buffer View root parameter that is used by the vertex shader.
			CD3DX12_ROOT_PARAMETER1 root_parameters[1];
			root_parameters[0].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_PIXEL);

			// We don't use another descriptor heap for the sampler, instead we use a static sampler
			CD3DX12_STATIC_SAMPLER_DESC samplers[1];
			samplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_POINT);

			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
			root_signature_desc.Init_1_1(_countof(root_parameters), root_parameters, 1, samplers, root_signature_flags);

			// Serialize the root signature.
			ID3DBlob* root_signature_blob;
			ID3DBlob* error_blob;
			ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&root_signature_desc,
																feature_data.HighestVersion, &root_signature_blob, &error_blob));

			// Create the root signature.
			ThrowIfFailed(g_RenderingDevice.GetD3DDevice()->CreateRootSignature(0, root_signature_blob->GetBufferPointer(),
																				root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc;
		::memset(&pso_desc, 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

		//void ShaderObject::CreatePSO(const D3D12_SHADER_BYTECODE inVSBytecode, const D3D12_SHADER_BYTECODE inPSBytecode)
		{
			// TODO: Create this from the Shader instead
			// Create the vertex input layout
			D3D12_INPUT_ELEMENT_DESC input_layout[] =
			{
				// Based on VertexPosUV
				{ "POSITION",	0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD",	0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};

			pso_desc.pRootSignature			= m_RootSignature;
			pso_desc.VS						= InlineShaders::TextureCopyVS;
			pso_desc.PS						= InlineShaders::TextureCopyPS;
			pso_desc.InputLayout			= { input_layout, _countof(input_layout) };
			pso_desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

			// What is this value? Documentation says: The sample mask for the blend state.
			pso_desc.SampleMask				= UINT_MAX;
		}

		// Setup PSO desc
		{
			// Setup blend states
			{
				pso_desc.BlendState.AlphaToCoverageEnable	= false;
				pso_desc.BlendState.IndependentBlendEnable	= false;

				const D3D12_RENDER_TARGET_BLEND_DESC default_render_target_blend_desc =
				{
					false, false,
					D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
					D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
					D3D12_LOGIC_OP_NOOP,
					D3D12_COLOR_WRITE_ENABLE_ALL,
				};

				for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
					pso_desc.BlendState.RenderTarget[i] = default_render_target_blend_desc;
			}

			// Setup rasterizer states
			{
				pso_desc.RasterizerState.FillMode				= D3D12_FILL_MODE_SOLID;
				pso_desc.RasterizerState.CullMode				= D3D12_CULL_MODE_BACK;
				pso_desc.RasterizerState.FrontCounterClockwise	= false;
				pso_desc.RasterizerState.DepthBias				= D3D12_DEFAULT_DEPTH_BIAS;
				pso_desc.RasterizerState.DepthBiasClamp			= D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
				pso_desc.RasterizerState.SlopeScaledDepthBias	= D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
				pso_desc.RasterizerState.DepthClipEnable		= true;
				pso_desc.RasterizerState.MultisampleEnable		= false;
				pso_desc.RasterizerState.AntialiasedLineEnable	= true;
				pso_desc.RasterizerState.ForcedSampleCount		= 0;
				pso_desc.RasterizerState.ConservativeRaster		= D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			}

			// Setup depth stencil states
			{
				const D3D12_DEPTH_STENCILOP_DESC default_stencil_op = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };

				pso_desc.DepthStencilState.DepthEnable		= false;
				pso_desc.DepthStencilState.DepthWriteMask	= D3D12_DEPTH_WRITE_MASK_ZERO;
				pso_desc.DepthStencilState.DepthFunc		= D3D12_COMPARISON_FUNC_NEVER;
				pso_desc.DepthStencilState.StencilEnable	= false;
				pso_desc.DepthStencilState.StencilReadMask	= D3D12_DEFAULT_STENCIL_READ_MASK;
				pso_desc.DepthStencilState.StencilWriteMask	= D3D12_DEFAULT_STENCIL_WRITE_MASK;

				pso_desc.DepthStencilState.FrontFace		= default_stencil_op;
				pso_desc.DepthStencilState.BackFace			= default_stencil_op;
			}

			pso_desc.NumRenderTargets = 1;

			// TODO: Grab appropriate format
			constexpr DXGI_FORMAT color_format = DXGI_FORMAT_R8G8B8A8_UNORM;
			constexpr DXGI_FORMAT depth_format = DXGI_FORMAT_UNKNOWN;

			::memset(&pso_desc.RTVFormats[0], DXGI_FORMAT_UNKNOWN, sizeof(pso_desc.RTVFormats));
			pso_desc.RTVFormats[0]	= color_format;
			pso_desc.DSVFormat		= depth_format;

			// Sample desc
			pso_desc.SampleDesc.Count	= 1;
			pso_desc.SampleDesc.Quality	= 0;
		}

		// Finally create the PSO
		ThrowIfFailed(g_RenderingDevice.GetD3DDevice()->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&m_PipelineState)));
	}
}

void DrawUtils::Destroy()
{
	m_PipelineState->Release();
	m_PipelineState = nullptr;
	m_RootSignature->Release();
	m_RootSignature = nullptr;
}

void SetupBindings(ID3D12GraphicsCommandList2* inCommandList, DX12RenderTarget* inRenderTarget)
{
	// TODO: This is whack
	auto& descriptor_heap = g_RenderingDevice.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetDescriptorHeap();
	uint32 descriptor_index = descriptor_heap.Allocate();
	D3D12_CPU_DESCRIPTOR_HANDLE descriptor_handle = descriptor_heap.GetCPUHandle(descriptor_index);

	// Create temporary SRV of the render target
	g_RenderingDevice.GetD3DDevice()->CreateShaderResourceView(inRenderTarget->GetResource(), nullptr, descriptor_handle);

	// Set slot 0 of our root signature to point to our descriptor heap with the texture SRV
	inCommandList->SetGraphicsRootDescriptorTable(0, descriptor_heap.GetGPUHandle(descriptor_index));
}

void DrawUtils::DrawFullScreenTriangle(ID3D12GraphicsCommandList2* inCommandList, DX12Resource* inTexture)
{
	DX12RenderTarget* render_target = dynamic_cast<DX12RenderTarget*>(inTexture);
	Assert(render_target != nullptr);

	s_FullScreenTriangle.Set(inCommandList);

	inCommandList->SetGraphicsRootSignature(m_RootSignature);
	inCommandList->SetPipelineState(m_PipelineState);

	// Transition from Render target to SRV
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			inTexture->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		inCommandList->ResourceBarrier(1, &barrier);
	}

	SetupBindings(inCommandList, render_target);

	inCommandList->DrawIndexedInstanced(3, 1, 0, 0, 0);

	// Transition from back to Render target
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			inTexture->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		inCommandList->ResourceBarrier(1, &barrier);
	}
}
