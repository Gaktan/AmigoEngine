#include "Engine.h"
#include "ShaderObject.h"

#include "DX12/DX12Device.h"

#include "Shaders/Include/Shaders.h"
#include "Shaders/Include/VertexLayouts.h"

ShaderObject::ShaderObject(RenderPass inRenderPass, const D3D12_SHADER_BYTECODE inVSBytecode, const D3D12_SHADER_BYTECODE inPSBytecode) :
	m_RenderPass(inRenderPass)
{
	CreateRootSignature();
	CreatePSO(inVSBytecode, inPSBytecode);
}

ShaderObject::~ShaderObject()
{
	m_PipelineState->Release();
	m_RootSignature->Release();
}

void ShaderObject::Set(ID3D12GraphicsCommandList2& inCommandList) const
{
	// TODO: Deal with actual shader bindings (textures, constant buffers, ...)
	inCommandList.SetGraphicsRootSignature(m_RootSignature);
	inCommandList.SetPipelineState(m_PipelineState);
}

void ShaderObject::CreatePSO(const D3D12_SHADER_BYTECODE inVSBytecode, const D3D12_SHADER_BYTECODE inPSBytecode)
{
	Assert(m_RootSignature != nullptr);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc;
	::memset(&pso_desc, 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	pso_desc.pRootSignature			= m_RootSignature;
	pso_desc.VS						= inVSBytecode;
	pso_desc.PS						= inPSBytecode;
	pso_desc.InputLayout			= { VertexInputLayouts::VertexPosUVNormal, _countof(VertexInputLayouts::VertexPosUVNormal) };
	pso_desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// What is this value? Documentation says: The sample mask for the blend state.
	pso_desc.SampleMask				= UINT_MAX;

	RenderPassDesc::SetupRenderPassDesc(m_RenderPass, pso_desc);

	ThrowIfFailed(g_RenderingDevice.GetD3DDevice().CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&m_PipelineState)));
}

void ShaderObject::CreateRootSignature()
{
	// TODO: Root signatures should be associated with the shader or shader bindings instead!

	// Create a root signature.
	D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};
	feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(g_RenderingDevice.GetD3DDevice().CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
	{
		feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS root_signature_flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	// Create a descriptor table with one entry in our descriptor heap
	CD3DX12_DESCRIPTOR_RANGE1 range { D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 };

	// A single Constant Buffer View root parameter that is used by the vertex shader.
	CD3DX12_ROOT_PARAMETER1 root_parameters[2];
	root_parameters[0].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_PIXEL);
	root_parameters[1].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);

	// We don't use another descriptor heap for the sampler, instead we use a static sampler
	CD3DX12_STATIC_SAMPLER_DESC samplers[1];
	samplers[0].Init(0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
	root_signature_desc.Init_1_1(_countof(root_parameters), root_parameters, 1, samplers, root_signature_flags);

	// Serialize the root signature.
	ID3DBlob* root_signature_blob;
	ID3DBlob* error_blob;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&root_signature_desc,
														feature_data.HighestVersion, &root_signature_blob, &error_blob));

	// Create the root signature.
	ThrowIfFailed(g_RenderingDevice.GetD3DDevice().CreateRootSignature(0, root_signature_blob->GetBufferPointer(),
																	   root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));
}