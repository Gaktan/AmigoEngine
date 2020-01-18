#include "Engine.h"
#include "DX12RenderTarget.h"

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
	ID3D12Device* inDevice,
	ui32 inWidth, ui32 inHeight, float inClearValue/* = 1.0f*/, DXGI_FORMAT inDepthFormat/* = DXGI_FORMAT_D24_UNORM_S8_UINT*/)
	: m_ClearValue(inClearValue)
	, m_Format(inDepthFormat)
{
	// Create the descriptor heap for the depth-stencil view.
	D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {};
	descriptor_heap_desc.NumDescriptors	= 1;
	descriptor_heap_desc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	descriptor_heap_desc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(inDevice->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&m_DSVHeap)));

	// Create a depth buffer with fast clear
	D3D12_CLEAR_VALUE optimized_clear_value = {};
	optimized_clear_value.Format		= inDepthFormat;
	optimized_clear_value.DepthStencil	= { inClearValue, 0 };

	D3D12_HEAP_PROPERTIES	heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC		resource_desc	= CD3DX12_RESOURCE_DESC::Tex2D(inDepthFormat, inWidth, inHeight,
																		   /*arraySize*/ 1, /*mipLevels*/ 0, /*sampleCount*/ 1, /*sampleQuality*/ 0,
																		   D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	ThrowIfFailed(inDevice->CreateCommittedResource(
		&heap_properties,
		D3D12_HEAP_FLAG_NONE,
		&resource_desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimized_clear_value,
		IID_PPV_ARGS(&m_Resource)
	));

	// Update the depth-stencil view.
	D3D12_DEPTH_STENCIL_VIEW_DESC view_desc = {};
	view_desc.Format				= inDepthFormat;
	view_desc.ViewDimension			= D3D12_DSV_DIMENSION_TEXTURE2D;
	view_desc.Texture2D.MipSlice	= 0;
	view_desc.Flags					= D3D12_DSV_FLAG_NONE;

	inDevice->CreateDepthStencilView(m_Resource, &view_desc, m_DSVHeap->GetCPUDescriptorHandleForHeapStart());
}

DX12DepthRenderTarget::~DX12DepthRenderTarget()
{
	m_DSVHeap->Release();
}

void DX12DepthRenderTarget::ClearDepth(ID3D12GraphicsCommandList2* inCommandList) const
{
	inCommandList->ClearDepthStencilView(GetCPUDescriptorHandle(), D3D12_CLEAR_FLAG_DEPTH, m_ClearValue, 0, 0, nullptr);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12DepthRenderTarget::GetCPUDescriptorHandle() const
{
	return m_DSVHeap->GetCPUDescriptorHandleForHeapStart();
}

DXGI_FORMAT DX12DepthRenderTarget::GetFormat() const
{
	return m_Format;
}
