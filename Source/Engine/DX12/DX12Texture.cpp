#include "Engine.h"
#include "DX12/DX12Texture.h"

#include "DX12/DX12DescriptorHeap.h"
#include "DX12/DX12Device.h"

DX12Texture::~DX12Texture()
{
	DX12DescriptorHeap* descriptor_heap = g_RenderingDevice.GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descriptor_heap->Release(m_CPUHandle);
}

void DX12Texture::InitAsTexture(
	ID3D12GraphicsCommandList2* inCommandList,
	uint32 inWidth, uint32 inHeight, DXGI_FORMAT inFormat,
	const void* inBufferData)
{
	m_Width		= inWidth;
	m_Height	= inHeight;
	m_Format	= inFormat;

	D3D12_HEAP_PROPERTIES	heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC		resource_desc	= CD3DX12_RESOURCE_DESC::Tex2D(m_Format, m_Width, m_Height, /*arraySize*/ 1, /*mipLevels*/ 1);

	ThrowIfFailed(g_RenderingDevice.GetD3DDevice()->CreateCommittedResource(
		&heap_properties,
		D3D12_HEAP_FLAG_NONE,
		&resource_desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_Resource)
	));

	UpdateBufferResource(inCommandList, 0, inBufferData);

	const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_Resource,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	inCommandList->ResourceBarrier(1, &barrier);

	// Create the shader resourcer view.
	D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = {};
	view_desc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE2D;
	view_desc.Shader4ComponentMapping		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	view_desc.Format						= inFormat;
	view_desc.Texture2D.MipLevels			= 1;
	view_desc.Texture2D.MostDetailedMip		= 0;
	view_desc.Texture2D.ResourceMinLODClamp	= 0.0f;

	// Allocate descriptors
	{
		DX12DescriptorHeap* descriptor_heap = g_RenderingDevice.GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		uint32 descriptor_index	= descriptor_heap->Allocate();
		m_CPUHandle = descriptor_heap->GetCPUHandle(descriptor_index);
	}

	g_RenderingDevice.GetD3DDevice()->CreateShaderResourceView(m_Resource, &view_desc, m_CPUHandle);
}

void DX12Texture::UpdateBufferResource(
	ID3D12GraphicsCommandList2* inCommandList,
	size_t inBufferSize/* = 0*/, const void* inBufferData/* = nullptr*/)
{
	// Make sure inBufferSize is ignored
	Assert(inBufferSize == 0);
	Assert(inBufferData != nullptr);
	Assert(m_IntermediateResource == nullptr);

	// Create an committed resource for the upload.
	D3D12_HEAP_PROPERTIES heap_properties	= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	// Compute total texture size
	const uint64 upload_buffer_size			= GetRequiredIntermediateSize(m_Resource, 0, 1);

	D3D12_RESOURCE_DESC resource_desc		= CD3DX12_RESOURCE_DESC::Buffer(upload_buffer_size);

	// Create upload buffer on the CPU
	ThrowIfFailed(g_RenderingDevice.GetD3DDevice()->CreateCommittedResource(
		&heap_properties,
		D3D12_HEAP_FLAG_NONE,
		&resource_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_IntermediateResource)));

	D3D12_SUBRESOURCE_DATA subresource_data = {};
	subresource_data.pData		= inBufferData;
	subresource_data.RowPitch	= m_Width * 4; // TODO: Change this based on format!!
	subresource_data.SlicePitch	= m_Width * m_Height * 4;

	UpdateSubresources(inCommandList,
						m_Resource, m_IntermediateResource,
						0, 0, 1, &subresource_data);

	SetResourceName(m_IntermediateResource, "DX12Texture::UpdateBufferResource");
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Texture::GetCPUHandle() const
{
	return m_CPUHandle;
}
