#include "engine_precomp.h"
#include "dx12rendertarget.h"

#include "D3dx12.h"

#if 0
DX12RenderTarget::DX12RenderTarget()
{
}

DX12RenderTarget::DX12RenderTarget(
	ID3D12Device* device,
	ID3D12GraphicsCommandList2* commandList,
	ui32 width, ui32 height, DXGI_FORMAT format,
	D3D12_RESOURCE_FLAGS flags/* = D3D12_RESOURCE_FLAG_NONE*/)
{
	// Create a committed resource for the GPU resource in a default heap.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_Resource)));



	UpdateBufferResource(device, commandList, bufferSize, bufferData, flags);
}

DX12RenderTarget::~DX12RenderTarget()
{
}
#endif

DX12DepthRenderTarget::DX12DepthRenderTarget(
	ID3D12Device* device,
	ui32 width, ui32 height, float clearValue/* = 1.0f*/, DXGI_FORMAT depthFormat/* = DXGI_FORMAT_D24_UNORM_S8_UINT*/)
	: m_ClearValue(clearValue)
	, m_Format(depthFormat)
{
	// Create the descriptor heap for the depth-stencil view.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap)));

	// Create a depth buffer with fast clear
	D3D12_CLEAR_VALUE optimizedClearValue = {};
	optimizedClearValue.Format = depthFormat;
	optimizedClearValue.DepthStencil = { clearValue, 0 };

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(depthFormat, width, height,
									  /*arraySize*/ 1, /*mipLevels*/ 0, /*sampleCount*/ 1, /*sampleQuality*/ 0,
									  D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimizedClearValue,
		IID_PPV_ARGS(&m_Resource)
	));

	// Update the depth-stencil view.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
	dsv.Format = depthFormat;
	dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv.Texture2D.MipSlice = 0;
	dsv.Flags = D3D12_DSV_FLAG_NONE;

	device->CreateDepthStencilView(m_Resource, &dsv, m_DSVHeap->GetCPUDescriptorHandleForHeapStart());
}

DX12DepthRenderTarget::~DX12DepthRenderTarget()
{
	m_DSVHeap->Release();
}

void DX12DepthRenderTarget::ClearDepth(ID3D12GraphicsCommandList2* commandList)
{
	commandList->ClearDepthStencilView(GetCPUDescriptorHandle(), D3D12_CLEAR_FLAG_DEPTH, m_ClearValue, 0, 0, nullptr);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12DepthRenderTarget::GetCPUDescriptorHandle()
{
	return m_DSVHeap->GetCPUDescriptorHandleForHeapStart();
}

DXGI_FORMAT DX12DepthRenderTarget::GetFormat()
{
	return m_Format;
}
