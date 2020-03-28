#include "Engine.h"
#include "DrawableObject.h"

#include "DX12/DX12Includes.h"

#include "Shaders/Include/Shaders.h"

#include "Gfx/Mesh.h"

DrawableObject::~DrawableObject()
{
	m_PipelineState->Release();
	m_RootSignature->Release();

	delete m_Mesh;
}

DrawableObject* DrawableObject::CreateDrawableObject(DX12Device& inDevice, Mesh* inMesh)
{
	DrawableObject* drawable_object = new DrawableObject();

	drawable_object->m_Mesh = inMesh;
	drawable_object->CreateRootSignature(inDevice);
	drawable_object->CreatePSO(inDevice);

	return drawable_object;
}

void DrawableObject::SetupBindings(ID3D12GraphicsCommandList2* inCommandList)
{
	// TODO: Deal with actual shader bindings (textures, constant buffers, ...)
	inCommandList->SetGraphicsRootSignature(m_RootSignature);
	inCommandList->SetPipelineState(m_PipelineState);

	m_Mesh->Set(inCommandList);
}

void DrawableObject::Render(ID3D12GraphicsCommandList2* inCommandList)
{
	inCommandList->DrawIndexedInstanced(m_Mesh->GetNumIndices(), 1, 0, 0, 0);
}

void DrawableObject::CreatePSO(DX12Device& inDevice)
{
	Assert(m_RootSignature != nullptr);

	auto* dx12_device	= inDevice.GetD3DDevice();

	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE			m_RootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT				m_InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY		m_PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS						m_VertexShader;
		CD3DX12_PIPELINE_STATE_STREAM_PS						m_PixelShader;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT		m_DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS		m_RTVFormats;
		CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER				m_Rasterizer;
	} pipeline_state_stream;

	// TODO: Grab appropriate format
	constexpr DXGI_FORMAT color_format = DXGI_FORMAT_R8G8B8A8_UNORM;
	constexpr DXGI_FORMAT depth_format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	D3D12_RT_FORMAT_ARRAY rtv_formats = {};
	rtv_formats.NumRenderTargets	= 1;
	rtv_formats.RTFormats[0]		= color_format;

	// TODO: Create this from the Shader instead
	// Create the vertex input layout
	D3D12_INPUT_ELEMENT_DESC input_layout[] =
	{
		// Based on VertexPosUVNormal
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	pipeline_state_stream.m_RootSignature			= m_RootSignature;
	pipeline_state_stream.m_InputLayout				= { input_layout, _countof(input_layout) };
	pipeline_state_stream.m_PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipeline_state_stream.m_VertexShader			= GET_SHADER_BYTECODE(VertexShader);
	pipeline_state_stream.m_PixelShader				= GET_SHADER_BYTECODE(PixelShader);
	pipeline_state_stream.m_DSVFormat				= depth_format;
	pipeline_state_stream.m_RTVFormats				= rtv_formats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipeline_state_desc =
	{
		sizeof(PipelineStateStream), &pipeline_state_stream
	};
	ThrowIfFailed(dx12_device->CreatePipelineState(&pipeline_state_desc, IID_PPV_ARGS(&m_PipelineState)));
}

void DrawableObject::CreateRootSignature(DX12Device& inDevice)
{
	// TODO: Root signatures should be associated with the shader or shader bindings instead!
	auto* dx12_device	= inDevice.GetD3DDevice();

	// Create a root signature.
	D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};
	feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(dx12_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
	{
		feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
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
	root_signature_desc.Init_1_1(_countof(root_parameters), root_parameters, 1, samplers, rootSignatureFlags);

	// Serialize the root signature.
	ID3DBlob* root_signature_blob;
	ID3DBlob* error_blob;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&root_signature_desc,
														feature_data.HighestVersion, &root_signature_blob, &error_blob));

	// Create the root signature.
	ThrowIfFailed(dx12_device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(),
												   root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));
}
