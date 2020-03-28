#include "Engine.h"
#include "DX12/DX12RenderTarget.h"

#include "D3dx12.h"

void DX12RenderTarget::InitAsRenderTarget(
	DX12Device& inDevice,
	D3D12_CPU_DESCRIPTOR_HANDLE inDescriptorHandle,
	uint32 inWidth, uint32 inHeight, Vec4 inClearValue/* = 1.0f*/,
	DXGI_FORMAT inFormat/* = DXGI_FORMAT_UNKNOWN*/)
{
	m_ClearValue		= inClearValue;
	m_Format			= inFormat;
	m_DescriptorHandle	= inDescriptorHandle;

	// Create a depth buffer with fast clear
	D3D12_CLEAR_VALUE optimized_clear_value = {};
	optimized_clear_value.Format		= m_Format;
	::memcpy(optimized_clear_value.Color, &m_ClearValue[0], sizeof(Vec4));

	D3D12_HEAP_PROPERTIES	heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC		resource_desc	= CD3DX12_RESOURCE_DESC::Tex2D(m_Format, inWidth, inHeight,
																		   /*arraySize*/ 1, /*mipLevels*/ 1, /*sampleCount*/ 1, /*sampleQuality*/ 0,
																		   D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

	ThrowIfFailed(inDevice.GetD3DDevice()->CreateCommittedResource(
		&heap_properties,
		D3D12_HEAP_FLAG_NONE,
		&resource_desc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&optimized_clear_value,
		IID_PPV_ARGS(&m_Resource)
	));

	// Update the render target view.
	D3D12_RENDER_TARGET_VIEW_DESC view_desc = {};
	view_desc.Format				= m_Format;
	view_desc.ViewDimension			= D3D12_RTV_DIMENSION_TEXTURE2D;
	view_desc.Texture2D.MipSlice	= 0;
	view_desc.Texture2D.PlaneSlice	= 0;

	inDevice.GetD3DDevice()->CreateRenderTargetView(m_Resource, &view_desc, m_DescriptorHandle);

	SetResourceName(m_Resource, "DX12RenderTarget::InitAsRenderTarget");
}

void DX12RenderTarget::InitFromResource(
	DX12Device& inDevice,
	ID3D12Resource* inResource,
	D3D12_CPU_DESCRIPTOR_HANDLE inDescriptorHandle,
	Vec4 inClearValue/* = 1.0f*/,
	DXGI_FORMAT inFormat/* = DXGI_FORMAT_D24_UNORM_S8_UINT*/)
{
	m_ClearValue		= inClearValue;
	m_Format			= inFormat;
	m_DescriptorHandle	= inDescriptorHandle;
	m_Resource			= inResource;

	inDevice.GetD3DDevice()->CreateRenderTargetView(m_Resource, nullptr, m_DescriptorHandle);

	SetResourceName(m_Resource, "DX12RenderTarget::InitFromResource");
}

void DX12RenderTarget::ClearBuffer(ID3D12GraphicsCommandList2* inCommandList) const
{
	inCommandList->ClearRenderTargetView(m_DescriptorHandle, &m_ClearValue[0], 0, nullptr);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12RenderTarget::GetCPUDescriptorHandle() const
{
	return m_DescriptorHandle;
}

DXGI_FORMAT DX12RenderTarget::GetFormat() const
{
	return m_Format;
}

void DX12DepthRenderTarget::InitAsDepthStencilBuffer(
	DX12Device& inDevice,
	D3D12_CPU_DESCRIPTOR_HANDLE inDescriptorHandle,
	uint32 inWidth, uint32 inHeight,
	float inClearValue/* = 1.0f*/, uint8 inStencilClearValue/* = 0*/,
	DXGI_FORMAT inDepthFormat/* = DXGI_FORMAT_D24_UNORM_S8_UINT*/)
{
	m_ClearValue		= { inClearValue, (float)inStencilClearValue, 0.0f, 0.0f };
	m_Format			= inDepthFormat;
	m_DescriptorHandle	= inDescriptorHandle;

	// Create a depth buffer with fast clear
	D3D12_CLEAR_VALUE optimized_clear_value = {};
	optimized_clear_value.Format		= m_Format;
	optimized_clear_value.DepthStencil	= { m_ClearValue[0], inStencilClearValue };

	D3D12_HEAP_PROPERTIES	heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC		resource_desc	= CD3DX12_RESOURCE_DESC::Tex2D(m_Format, inWidth, inHeight,
																		   /*arraySize*/ 1, /*mipLevels*/ 1, /*sampleCount*/ 1, /*sampleQuality*/ 0,
																		   D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	ThrowIfFailed(inDevice.GetD3DDevice()->CreateCommittedResource(
		&heap_properties,
		D3D12_HEAP_FLAG_NONE,
		&resource_desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimized_clear_value,
		IID_PPV_ARGS(&m_Resource)
	));

	// Update the depth-stencil view.
	D3D12_DEPTH_STENCIL_VIEW_DESC view_desc = {};
	view_desc.Format				= m_Format;
	view_desc.ViewDimension			= D3D12_DSV_DIMENSION_TEXTURE2D;
	view_desc.Texture2D.MipSlice	= 0;
	view_desc.Flags					= D3D12_DSV_FLAG_NONE;

	inDevice.GetD3DDevice()->CreateDepthStencilView(m_Resource, &view_desc, m_DescriptorHandle);

	SetResourceName(m_Resource, "DX12DepthRenderTarget::InitAsDepthStencilBuffer");
}

void DX12DepthRenderTarget::ClearBuffer(ID3D12GraphicsCommandList2* inCommandList) const
{
	inCommandList->ClearDepthStencilView(m_DescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, m_ClearValue[0], (uint8)m_ClearValue[1], 0, nullptr);
}