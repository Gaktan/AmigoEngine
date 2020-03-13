#include "Engine.h"
#include "Test.h"

#include <iostream>

// This file will be used to prototype.
// Once a feature is complete, we can create additional files
// based on this one

#include "Math/Math.h"
#include "Math/Vec4.h"
#include "Math/Mat4.h"

#include "DX12/DX12Device.h"
#include "DX12/DX12Resource.h"
#include "DX12/DX12RenderTarget.h"
#include "DX12/DX12CommandQueue.h"

#include "Gfx/Mesh.h"
#include "Gfx/MeshLoader.h"

#include "Shaders/Include/ConstantBuffers.h"
#include "Shaders/Include/Shaders.h"

// Mesh for the scene
Mesh* m_SceneMesh;

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

Mat4	m_ModelMatrix;
Mat4	m_ViewMatrix;
Mat4	m_ProjectionMatrix;
float	m_ColorMul;
IVec4	m_Color;

bool m_ContentLoaded;

void ResizeBuffers(DX12Device& inDevice, int inNewWidth, int inNewHeight)
{
	// Flush any GPU commands that might be referencing the depth buffer.
	inDevice.Flush();

	if (m_DepthBuffer)
	{
		delete m_DepthBuffer;
	}
	m_DepthBuffer = new DX12DepthRenderTarget();

	DX12DescriptorHeap descriptor_heap = inDevice.GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsv_handle(descriptor_heap.m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	m_DepthBuffer->InitAsDepthStencilBuffer(inDevice, dsv_handle, inNewWidth, inNewHeight);
}

bool LoadContent(DX12Device& inDevice, uint32 inWidth, uint32 inHeight)
{
	{
		m_ScissorRect	= CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
		m_Viewport		= CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(inWidth), static_cast<float>(inHeight));
		m_FOV			= 45.0f;
		m_ContentLoaded = false;
	}

	MeshLoader::Init();

	auto dx12_device	= inDevice.m_Device;
	auto command_queue	= inDevice.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT); // Don't use COPY for this.
	auto command_list	= command_queue->GetCommandList(inDevice);

	// Create the depth buffer
	ResizeBuffers(inDevice, inWidth, inHeight);

	// Create the vertex input layout
	D3D12_INPUT_ELEMENT_DESC input_layout[] =
	{
		// Based on VertexPosUVNormal
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	MeshLoader mesh_loader;
	mesh_loader.LoadFromFile("Data\\Cornell_fake_box.obj");
	m_SceneMesh = mesh_loader.CreateMeshObject(dx12_device, command_list);

	// Create Constant Buffer View
	m_ConstantBuffer = new DX12ConstantBuffer();
	m_ConstantBuffer->InitAsConstantBuffer(dx12_device, sizeof(ConstantBuffer::ModelViewProjection));

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
		CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER				m_Rasterizer;
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

	delete m_SceneMesh;
	delete m_ConstantBuffer;

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

		ResizeBuffers(inDevice, inWidth, inHeight);
	}
}

float TTT = 0.0f;

void OnUpdate(uint32 inWidth, uint32 inHeight, float inDeltaT)
{
	TTT += inDeltaT;

	// Update the model matrix.
	float angle = TTT * 0.1f;
	const Vec4 rotation_axis(0, 1, 0, 0);
	m_ModelMatrix = Mat4::CreateRotationMatrix(rotation_axis, Math::ToRadians(180.0f));

	Vec4 position(0, Math::Sin(angle*0.01f) - 3.0f, 0);
	m_ModelMatrix = m_ModelMatrix.Tanslate(position);

	float radius = 10.0f;
	float x = Math::Cos(angle * 0.005f) * radius;
	float y = Math::Sin(angle * 0.005f) * radius;

	// Update the view matrix.
	const Vec4 eye_position(10, 3, y, 0);
	const Vec4 focus_point(0, 0, 0, 0);
	const Vec4 up_direction(0, 1, 0, 0);
	m_ViewMatrix = Mat4::CreateLookAtMatrix(eye_position, focus_point, up_direction);

	// Update the projection matrix.
	float aspect_ratio = inWidth / static_cast<float>(inHeight);
	m_ProjectionMatrix = Mat4::CreatePerspectiveMatrix(Math::ToRadians(m_FOV), aspect_ratio, 0.1f, 100.0f);

	m_ColorMul = Math::Abs(Math::Sin(TTT*0.001f));

	m_Color = (int32) Math::Abs(Math::Sin(TTT*0.002f + 0.5f)) * 255;
}

void OnRender(DX12Device& inDevice)
{
	auto command_queue	= inDevice.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto command_list	= command_queue->GetCommandList(inDevice);

	// Clear the render targets.
	{
		inDevice.m_SwapChain->ClearBackBuffer(command_list);
		m_DepthBuffer->ClearBuffer(command_list);
	}

	command_list->SetPipelineState(m_PipelineState);
	command_list->SetGraphicsRootSignature(m_RootSignature);

	m_SceneMesh->Set(command_list);

	command_list->RSSetViewports(1, &m_Viewport);
	command_list->RSSetScissorRects(1, &m_ScissorRect);

	inDevice.m_SwapChain->SetRenderTarget(command_list, m_DepthBuffer);

	// TODO: Pre multiply matrix
	ConstantBuffer::ModelViewProjection mvp;
	mvp.Model		= m_ModelMatrix;
	mvp.View		= m_ViewMatrix;
	mvp.Projection	= m_ProjectionMatrix;

	mvp.ColorMul = Vec4(m_ColorMul);

	// Upload Constant Buffer to GPU
	m_ConstantBuffer->UpdateBufferResource(inDevice.m_Device, command_list, sizeof(ConstantBuffer::ModelViewProjection), &mvp);
	m_ConstantBuffer->SetConstantBuffer(command_list, 0);

	command_list->DrawIndexedInstanced(m_SceneMesh->GetNumIndices(), 1, 0, 0, 0);

	// Present
	inDevice.Present(command_list);
}
