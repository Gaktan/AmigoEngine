#include "Engine.h"
#include "Test.h"

// This file will be used to prototype.
// Once a feature is complete, we can create additional files
// based on this one

#include "Math/Math.h"
#include "Math/Vector4.h"
#include "Math/Matrix4.h"

#include "DX12/DX12Device.h"
#include "DX12/DX12Resource.h"
#include "DX12/DX12RenderTarget.h"
#include "DX12/DX12CommandQueue.h"

#include "Shaders/Include/ConstantBuffers.h"
#include "Shaders/Include/Shaders.h"


// Vertex buffer for the cube.
DX12VertexBuffer* m_VertexBuffer;
// Index buffer for the cube.
DX12IndexBuffer* m_IndexBuffer;
// Constant Buffer for the vertex shader
DX12ConstantBuffer* m_ConstantBuffer;

// Depth buffer.
DX12DepthRenderTarget* m_DepthBuffer = nullptr;

// Root signature
ID3D12RootSignature* m_RootSignature;

// Pipeline state object.
ID3D12PipelineState* m_PipelineState;

D3D12_VIEWPORT m_Viewport;
D3D12_RECT m_ScissorRect;

float m_FOV;

Matrix4f	m_ModelMatrix;
Matrix4f	m_ViewMatrix;
Matrix4f	m_ProjectionMatrix;
float		m_ColorMul;

bool m_ContentLoaded;

// Vertex data for a colored cube.
struct VertexPosColor
{
	Vector4f Position;
	Vector4f Color;
};

static VertexPosColor g_Vertices[8] =
{
	{ Vector4f(-1.0f, -1.0f, -1.0f), Vector4f(0.0f, 0.0f, 0.0f) }, // 0
	{ Vector4f(-1.0f,  1.0f, -1.0f), Vector4f(0.0f, 1.0f, 0.0f) }, // 1
	{ Vector4f( 1.0f,  1.0f, -1.0f), Vector4f(1.0f, 1.0f, 0.0f) }, // 2
	{ Vector4f( 1.0f, -1.0f, -1.0f), Vector4f(1.0f, 0.0f, 0.0f) }, // 3
	{ Vector4f(-1.0f, -1.0f,  1.0f), Vector4f(0.0f, 0.0f, 1.0f) }, // 4
	{ Vector4f(-1.0f,  1.0f,  1.0f), Vector4f(0.0f, 1.0f, 1.0f) }, // 5
	{ Vector4f( 1.0f,  1.0f,  1.0f), Vector4f(1.0f, 1.0f, 1.0f) }, // 6
	{ Vector4f( 1.0f, -1.0f,  1.0f), Vector4f(1.0f, 0.0f, 1.0f) }  // 7
};

static WORD g_Indicies[36] =
{
	0, 1, 2, 0, 2, 3,
	4, 6, 5, 4, 7, 6,
	4, 5, 1, 4, 1, 0,
	3, 2, 6, 3, 6, 7,
	1, 5, 6, 1, 6, 2,
	4, 0, 3, 4, 3, 7
};

void ResizeDepthBuffer(DX12Device& inDevice, int inWidth, int inHeight)
{
	// Flush any GPU commands that might be referencing the depth buffer.
	inDevice.Flush();

	// Resize screen dependent resources.
	// Create a depth buffer
	if (m_DepthBuffer)
	{
		delete m_DepthBuffer;
	}

	m_DepthBuffer = new DX12DepthRenderTarget(inDevice.m_Device, inWidth, inHeight);
}

bool LoadContent(DX12Device& inDevice, uint32 inWidth, uint32 inHeight)
{
	{
		m_ScissorRect	= CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
		m_Viewport		= CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(inWidth), static_cast<float>(inHeight));
		m_FOV			= 45.0f;
		m_ContentLoaded = false;
	}

	auto dx12_device	= inDevice.m_Device;
	auto command_queue	= inDevice.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT); // Don't use COPY for this.
	auto command_list	= command_queue->GetCommandList(&inDevice);

	// Create the vertex input layout
	D3D12_INPUT_ELEMENT_DESC input_layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	// Create vertex/index buffer data.
	m_VertexBuffer	= new DX12VertexBuffer(dx12_device, command_list, _countof(g_Vertices) * sizeof(VertexPosColor), g_Vertices, sizeof(VertexPosColor));
	m_IndexBuffer	= new DX12IndexBuffer(dx12_device, command_list, _countof(g_Indicies) * sizeof(WORD), g_Indicies);

	// Create Constant Buffer View
	m_ConstantBuffer = new DX12ConstantBuffer(dx12_device, command_list, sizeof(ConstantBuffer::ModelViewProjection), nullptr);

	// Create the depth buffer.
	ResizeDepthBuffer(inDevice, inWidth, inHeight);

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
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	// A single Constant Buffer View root parameter that is used by the vertex shader.
	CD3DX12_ROOT_PARAMETER1 root_parameters[1];
	root_parameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
	root_signature_desc.Init_1_1(_countof(root_parameters), root_parameters, 0, nullptr, rootSignatureFlags);

	// Serialize the root signature.
	ID3DBlob* root_signature_blob;
	ID3DBlob* error_blob;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&root_signature_desc,
														feature_data.HighestVersion, &root_signature_blob, &error_blob));
	// Create the root signature.
	ThrowIfFailed(dx12_device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(),
												   root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE			m_RootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT				m_InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY		m_PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS						m_VertexShader;
		CD3DX12_PIPELINE_STATE_STREAM_PS						m_PixelShader;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT		m_DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS		m_RTVFormats;
	} pipeline_state_stream;

	D3D12_RT_FORMAT_ARRAY rtv_formats = {};
	rtv_formats.NumRenderTargets	= 1;
	rtv_formats.RTFormats[0]		= DXGI_FORMAT_R8G8B8A8_UNORM;

	pipeline_state_stream.m_RootSignature			= m_RootSignature;
	pipeline_state_stream.m_InputLayout				= { input_layout, _countof(input_layout) };
	pipeline_state_stream.m_PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipeline_state_stream.m_VertexShader			= GET_SHADER_BYTECODE(VertexShader);
	pipeline_state_stream.m_PixelShader				= GET_SHADER_BYTECODE(PixelShader);
	pipeline_state_stream.m_DSVFormat				= m_DepthBuffer->GetFormat();
	pipeline_state_stream.m_RTVFormats				= rtv_formats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipeline_state_desc =
	{
		sizeof(PipelineStateStream), &pipeline_state_stream
	};
	ThrowIfFailed(dx12_device->CreatePipelineState(&pipeline_state_desc, IID_PPV_ARGS(&m_PipelineState)));

	auto fence_value = command_queue->ExecuteCommandList(command_list);
	command_queue->WaitForFenceValue(fence_value);

	m_ContentLoaded = true;

	return true;
}

void UnloadContent(DX12Device& inDevice)
{
	// Make sure the command queue has finished all commands before closing.
	inDevice.Flush();

	delete m_VertexBuffer;
	delete m_IndexBuffer;

	delete m_DepthBuffer;
	m_RootSignature->Release();
	m_PipelineState->Release();

	m_ContentLoaded = false;
}

void OnResize(DX12Device& inDevice, uint32 inWidth, uint32 inHeight)
{
	//if (inWidth != GetClientWidth() || inHeight != GetClientHeight())
	//if (inWidth != 800 || inHeight != 600)
	{
		inWidth		= Math::Max(256u, inWidth);
		inHeight	= Math::Max(256u, inHeight);
		m_Viewport	= CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(inWidth), static_cast<float>(inHeight));

		ResizeDepthBuffer(inDevice, inWidth, inHeight);
	}
}

float TTT = 0.0f;

void OnUpdate(uint32 inWidth, uint32 inHeight, float inDeltaT)
{
	TTT += inDeltaT;

	// Update the model matrix.
	float angle = TTT * 0.1f;
	const Vector4f rotation_axis(0, 1, 1, 0);
	m_ModelMatrix = Matrix4f::CreateRotationMatrix(rotation_axis, Math::ToRadians(angle));

	Vector4f position(0, 0, Math::Sin(angle*0.01f));
	m_ModelMatrix = m_ModelMatrix.Tanslate(position);

	// Update the view matrix.
	const Vector4f eye_position(0, -10, 0, 1);
	const Vector4f focus_point(0, 0, 0, 1);
	const Vector4f up_direction(0, 0, 1, 0);
	m_ViewMatrix = Matrix4f::CreateLookAtMatrix(eye_position, focus_point, up_direction);

	// Update the projection matrix.
	float aspect_ratio = inWidth / static_cast<float>(inHeight);
	m_ProjectionMatrix = Matrix4f::CreatePerspectiveMatrix(Math::ToRadians(m_FOV), aspect_ratio, 0.1f, 100.0f);

	m_ColorMul = Math::Abs(Math::Sin(TTT*0.001));
}

void OnRender(DX12Device& inDevice)
{
	auto command_queue	= inDevice.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto command_list	= command_queue->GetCommandList(&inDevice);

	// Clear the render targets.
	{
		inDevice.m_SwapChain->ClearBackBuffer(command_list);
		m_DepthBuffer->ClearDepth(command_list);
	}

	command_list->SetPipelineState(m_PipelineState);
	command_list->SetGraphicsRootSignature(m_RootSignature);

	command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_VertexBuffer->SetVertexBuffer(command_list, 0, 1);
	m_IndexBuffer->SetIndexBuffer(command_list);

	command_list->RSSetViewports(1, &m_Viewport);
	command_list->RSSetScissorRects(1, &m_ScissorRect);

	inDevice.m_SwapChain->SetRenderTarget(command_list, m_DepthBuffer);

	// Update the MVP matrix
	Matrix4f mvp_matrix = m_ModelMatrix.Mul(m_ViewMatrix);
	mvp_matrix			= mvp_matrix.Mul(m_ProjectionMatrix);

	// Upload Constant Buffer to GPU
	ConstantBuffer::ModelViewProjection mvp;
	memcpy(&mvp.MVP[0], &mvp_matrix, sizeof(ConstantBuffer::ModelViewProjection));
	mvp.ColorMul = Vector4f(m_ColorMul);
	m_ConstantBuffer->UpdateBufferResource(inDevice.m_Device, command_list, sizeof(ConstantBuffer::ModelViewProjection), &mvp);
	m_ConstantBuffer->SetConstantBuffer(command_list, 0);

	command_list->DrawIndexedInstanced(_countof(g_Indicies), 1, 0, 0, 0);

	// Present
	inDevice.Present(command_list);
}
