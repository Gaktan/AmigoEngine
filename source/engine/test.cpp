#include "engine_precomp.h"
#include "test.h"

// This file will be used to prototype.
// Once a feature is complete, we can create additional files
// based on this one

#include "math/math.h"
#include "math/vector4.h"
#include "math/matrix4.h"

#include "dx12/dx12device.h"
#include "dx12/dx12resource.h"
#include "dx12/dx12rendertarget.h"
#include "dx12/dx12commandqueue.h"

#include "shaders/include/shaders.h"


// Vertex buffer for the cube.
DX12VertexBuffer* m_VertexBuffer;
// Index buffer for the cube.
DX12IndexBuffer* m_IndexBuffer;

// Depth buffer.
DX12DepthRenderTarget* m_DepthBuffer = nullptr;

// Root signature
ID3D12RootSignature* m_RootSignature;

// Pipeline state object.
ID3D12PipelineState* m_PipelineState;

D3D12_VIEWPORT m_Viewport;
D3D12_RECT m_ScissorRect;

float m_FoV;

Matrix4f m_ModelMatrix;
Matrix4f m_ViewMatrix;
Matrix4f m_ProjectionMatrix;

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

void ResizeDepthBuffer(DX12Device& device, int width, int height)
{
	// Flush any GPU commands that might be referencing the depth buffer.
	device.Flush();

	// Resize screen dependent resources.
	// Create a depth buffer
	if (m_DepthBuffer)
	{
		delete m_DepthBuffer;
	}

	m_DepthBuffer = new DX12DepthRenderTarget(device.m_Device, width, height);
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
	auto commandQueue = dx12Device.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT); // Don't use COPY for this.
	auto commandList = commandQueue->GetCommandList(&dx12Device);

	// Create the vertex input layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	// Upload vertex buffer data.
	m_VertexBuffer = new DX12VertexBuffer(device, commandList, _countof(g_Vertices) * sizeof(VertexPosColor), g_Vertices, sizeof(VertexPosColor));
	m_IndexBuffer = new DX12IndexBuffer(device, commandList, _countof(g_Indicies) * sizeof(WORD), g_Indicies);

	// Create the depth buffer.
	ResizeDepthBuffer(dx12Device, width, height);

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
	rootParameters[0].InitAsConstants(sizeof(Matrix4f) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

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
	pipelineStateStream.VS = GET_SHADER_BYTECODE(VertexShader);
	pipelineStateStream.PS = GET_SHADER_BYTECODE(PixelShader);
	pipelineStateStream.DSVFormat = m_DepthBuffer->GetFormat();
	pipelineStateStream.RTVFormats = rtvFormats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc =
	{
		sizeof(PipelineStateStream), &pipelineStateStream
	};
	ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));

	auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);

	m_ContentLoaded = true;

	return true;
}

void UnloadContent(DX12Device& dx12Device)
{
	// Make sure the command queue has finished all commands before closing.
	dx12Device.Flush();

	delete m_VertexBuffer;
	delete m_IndexBuffer;

	delete m_DepthBuffer;
	m_RootSignature->Release();
	m_PipelineState->Release();

	m_ContentLoaded = false;
}

void OnResize(DX12Device& device, ui32 width, ui32 height)
{
	//if (width != GetClientWidth() || height != GetClientHeight())
	//if (width != 800 || height != 600)
	{
		width	= Math::Max(256u, width);
		height	= Math::Max(256u, height);

		m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
		ResizeDepthBuffer(device, width, height);
	}
}

float TTT = 0.0f;

void OnUpdate(ui32 width, ui32 height, float delta)
{
	TTT += delta;

	// Update the model matrix.
	float angle = TTT * 0.1f;
	const Vector4f rotationAxis(0, 1, 1, 0);
	m_ModelMatrix = Matrix4f::CreateRotationMatrix(rotationAxis, Math::ToRadians(angle));

	Vector4f position(0, 0, sinf(angle*0.01f));
	m_ModelMatrix = m_ModelMatrix.Tanslate(position);

	// Update the view matrix.
	const Vector4f eyePosition(0, -10, 0, 1);
	const Vector4f focusPoint(0, 0, 0, 1);
	const Vector4f upDirection(0, 0, 1, 0);
	m_ViewMatrix = Matrix4f::CreateLookAtMatrix(eyePosition, focusPoint, upDirection);

	// Update the projection matrix.
	float aspectRatio = width / static_cast<float>(height);
	m_ProjectionMatrix = Matrix4f::CreatePerspectiveMatrix(Math::ToRadians(m_FoV), aspectRatio, 0.1f, 100.0f);
}

void OnRender(DX12Device& dx12Device)
{
	auto commandQueue = dx12Device.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto commandList = commandQueue->GetCommandList(&dx12Device);

	// Clear the render targets.
	{
		dx12Device.m_SwapChain->ClearBackBuffer(commandList);
		m_DepthBuffer->ClearDepth(commandList);
	}

	commandList->SetPipelineState(m_PipelineState);
	commandList->SetGraphicsRootSignature(m_RootSignature);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_VertexBuffer->SetVertexBuffer(commandList, 0, 1);
	m_IndexBuffer->SetIndexBuffer(commandList);

	commandList->RSSetViewports(1, &m_Viewport);
	commandList->RSSetScissorRects(1, &m_ScissorRect);

	dx12Device.m_SwapChain->SetRenderTarget(commandList, m_DepthBuffer);

	// Update the MVP matrix
	Matrix4f mvpMatrix = m_ModelMatrix.Mul(m_ViewMatrix);
	mvpMatrix = mvpMatrix.Mul(m_ProjectionMatrix);

	commandList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix4f) / 4, &mvpMatrix, 0);

	commandList->DrawIndexedInstanced(_countof(g_Indicies), 1, 0, 0, 0);

	// Present
	dx12Device.Present(commandList);
}
