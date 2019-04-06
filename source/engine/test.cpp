#include "engine_precomp.h"
#include "test.h"

// This file will be used to prototype.
// Once a feature is complete, we can create additional files
// based on this one

#include <d3dcompiler.h>

#include <DirectXMath.h>

#include "math/math.h"
//#include "math/vector4.h"
//#include "math/matrix4.h"

#include "dx12/dx12device.h"

using namespace DirectX;

// Vertex buffer for the cube.
ID3D12Resource* m_VertexBuffer;
D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
// Index buffer for the cube.
ID3D12Resource* m_IndexBuffer;
D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

// Depth buffer.
ID3D12Resource* m_DepthBuffer;
// Descriptor heap for depth buffer.
ID3D12DescriptorHeap* m_DSVHeap;

// Root signature
ID3D12RootSignature* m_RootSignature;

// Pipeline state object.
ID3D12PipelineState* m_PipelineState;

D3D12_VIEWPORT m_Viewport;
D3D12_RECT m_ScissorRect;

float m_FoV;

XMMATRIX m_ModelMatrix;
XMMATRIX m_ViewMatrix;
XMMATRIX m_ProjectionMatrix;

bool m_ContentLoaded;

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

// Vertex data for a colored cube.
struct VertexPosColor
{
	XMFLOAT3 Position;
	XMFLOAT3 Color;
};

static VertexPosColor g_Vertices[8] = {
	{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
	{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
	{ XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
	{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
	{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
	{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
	{ XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
	{ XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
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

void UpdateBufferResource(
	ID3D12Device* device,
	ID3D12GraphicsCommandList2* commandList,
	ID3D12Resource** pDestinationResource,
	ID3D12Resource** pIntermediateResource,
	size_t numElements, size_t elementSize, const void* bufferData,
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE)
{
	size_t bufferSize = numElements * elementSize;

	// Create a committed resource for the GPU resource in a default heap.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(pDestinationResource)));

	// Create an committed resource for the upload.
	if (bufferData)
	{
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(pIntermediateResource)));

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = bufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		UpdateSubresources(commandList,
						   *pDestinationResource, *pIntermediateResource,
						   0, 0, 1, &subresourceData);
	}
}

void ResizeDepthBuffer(DX12Device& device, int width, int height)
{
	if (m_ContentLoaded)
	{
		// Flush any GPU commands that might be referencing the depth buffer.
		device.Flush();

		width = max(1, width);
		height = max(1, height);

		// Resize screen dependent resources.
		// Create a depth buffer.
		D3D12_CLEAR_VALUE optimizedClearValue = {};
		optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		optimizedClearValue.DepthStencil = { 1.0f, 0 };

		ThrowIfFailed(device.m_Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height,
										  1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&optimizedClearValue,
			IID_PPV_ARGS(&m_DepthBuffer)
		));

		// Update the depth-stencil view.
		D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
		dsv.Format = DXGI_FORMAT_D32_FLOAT;
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv.Texture2D.MipSlice = 0;
		dsv.Flags = D3D12_DSV_FLAG_NONE;

		device.m_Device->CreateDepthStencilView(m_DepthBuffer, &dsv,
												m_DSVHeap->GetCPUDescriptorHandleForHeapStart());
	}
}

bool LoadContent(DX12Device& dx12Device, ui32 width, ui32 height)
{
	{
		m_ScissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
		m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
		m_FoV = 45.0f;
		m_ContentLoaded = false;
	}
	auto device = dx12Device.m_Device;
	//auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	//auto commandList = commandQueue->GetCommandList();
	auto commandList = dx12Device.m_CommandList;
	auto commandQueue = dx12Device.m_CommandQueue;

	// Upload vertex buffer data.
	ID3D12Resource* intermediateVertexBuffer;
	UpdateBufferResource(device, commandList,
						 &m_VertexBuffer, &intermediateVertexBuffer,
						 _countof(g_Vertices), sizeof(VertexPosColor), g_Vertices);

	// Create the vertex buffer view.
	m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes = sizeof(g_Vertices);
	m_VertexBufferView.StrideInBytes = sizeof(VertexPosColor);

	// Upload index buffer data.
	ID3D12Resource* intermediateIndexBuffer;
	UpdateBufferResource(device, commandList,
						 &m_IndexBuffer, &intermediateIndexBuffer,
						 _countof(g_Indicies), sizeof(WORD), g_Indicies);

	// Create index buffer view.
	m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
	m_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	m_IndexBufferView.SizeInBytes = sizeof(g_Indicies);

	// Create the descriptor heap for the depth-stencil view.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap)));

#if 0
	// Load the vertex shader.
	ID3DBlob* vertexShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderBlob));

	// Load the pixel shader.
	ID3DBlob* pixelShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"PixelShader.cso", &pixelShaderBlob));
#else
	ID3DBlob* vertexShaderBlob = nullptr;
	/*ThrowIfFailed*/HRESULT res = (D3DReadFileToBlob(L"D:/Programming/AmigoEngine/projects/engine/output/win64/debug/VertexShader.cso", &vertexShaderBlob));
	ThrowIfFailed(res);

	// Load the pixel shader.
	ID3DBlob* pixelShaderBlob = nullptr;
	/*ThrowIfFailed*/(D3DReadFileToBlob(L"D:/Programming/AmigoEngine/projects/engine/output/win64/debug/PixelShader.cso", &pixelShaderBlob));

#endif

	// Create the vertex input layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	// Create a root signature.
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	// A single 32-bit constant root parameter that is used by the vertex shader.
	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

	// Serialize the root signature.
	ID3DBlob* rootSignatureBlob;
	ID3DBlob* errorBlob;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
														featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
	// Create the root signature.
	ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
											  rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	} pipelineStateStream;

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	pipelineStateStream.pRootSignature = m_RootSignature;
	pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob);
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob);
	pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateStream.RTVFormats = rtvFormats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
		sizeof(PipelineStateStream), &pipelineStateStream
	};
	ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));

	//auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	//commandQueue->WaitForFenceValue(fenceValue);
	commandList->Close();
	ID3D12CommandList* const commandLists[] =
	{
		commandList
	};
	commandQueue->ExecuteCommandLists(1, commandLists);

	m_ContentLoaded = true;

	// Resize/Create the depth buffer.
	ResizeDepthBuffer(dx12Device, width, height);

	return true;
}

void OnResize(DX12Device& device, ui32 width, ui32 height)
{
	//if (width != GetClientWidth() || height != GetClientHeight())
	//if (width != 800 || height != 600)
	{
		m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));

		ResizeDepthBuffer(device, width, height);
	}
}

void UnloadContent()
{
	m_ContentLoaded = false;
}

float TTT = 0.0f;

void OnUpdate(ui32 width, ui32 height)
{
	TTT += 1000.0f / 120.0f;

	// Update the model matrix.
	float angle = TTT * 0.1f;
	const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
	m_ModelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

	// Update the view matrix.
	const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
	const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
	const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
	m_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

	// Update the projection matrix.
	float aspectRatio = width / static_cast<float>(height);
	m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_FoV), aspectRatio, 0.1f, 100.0f);
}

// Transition a resource
void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
								   Microsoft::WRL::ComPtr<ID3D12Resource> resource,
								   D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		resource.Get(),
		beforeState, afterState);

	commandList->ResourceBarrier(1, &barrier);
}

// Clear a render target.
void ClearRTV(ID3D12GraphicsCommandList2* commandList,
			  D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor)
{
	commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void ClearDepth(ID3D12GraphicsCommandList2* commandList,
				D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f)
{
	commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void OnRender(DX12Device& dx12Device)
{
	//auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	//auto commandList = commandQueue->GetCommandList();

	//UINT currentBackBufferIndex = m_pWindow->GetCurrentBackBufferIndex();
	//auto backBuffer = m_pWindow->GetCurrentBackBuffer();
	//auto rtv = m_pWindow->GetCurrentRenderTargetView();
	auto dsv = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(dx12Device.m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), dx12Device.m_CurrentBackBufferIndex, dx12Device.m_RTVDescriptorSize);

	dx12Device.TempRendering();
	auto commandList = dx12Device.m_CommandList;

	// Clear the render targets.
	{
		//TransitionResource(commandList, backBuffer,
		//				   D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		//
		//FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
		//
		//ClearRTV(commandList, rtv, clearColor);
		ClearDepth(commandList, dsv);
	}

	commandList->SetPipelineState(m_PipelineState);
	commandList->SetGraphicsRootSignature(m_RootSignature);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	commandList->IASetIndexBuffer(&m_IndexBufferView);

	commandList->RSSetViewports(1, &m_Viewport);
	commandList->RSSetScissorRects(1, &m_ScissorRect);

	commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	// Update the MVP matrix
	XMMATRIX mvpMatrix = XMMatrixMultiply(m_ModelMatrix, m_ViewMatrix);
	mvpMatrix = XMMatrixMultiply(mvpMatrix, m_ProjectionMatrix);
	commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

	commandList->DrawIndexedInstanced(_countof(g_Indicies), 1, 0, 0, 0);

	// Present
	dx12Device.Present();
}
