#include "Engine.h"
#include "DX12Resource.h"

#include "D3dx12.h"

DX12Resource::DX12Resource()
	: m_IntermediateResource(nullptr)
{
}

DX12Resource::DX12Resource(
	ID3D12Device* inDevice,
	ID3D12GraphicsCommandList2* inCommandList,
	size_t inBufferSize/* = 0*/, const void* inBufferData/* = nullptr*/,
	D3D12_RESOURCE_FLAGS inFlags/* = D3D12_RESOURCE_FLAG_NONE*/)
{
	D3D12_HEAP_PROPERTIES	heap_properties	= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC		resource_desc	= CD3DX12_RESOURCE_DESC::Buffer(inBufferSize, inFlags);

	// Create a committed resource for the GPU resource in a default heap.
	ThrowIfFailed(inDevice->CreateCommittedResource(
		&heap_properties,
		D3D12_HEAP_FLAG_NONE,
		&resource_desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_Resource)));

	UpdateBufferResource(inDevice, inCommandList, inBufferSize, inBufferData);
}

DX12Resource::~DX12Resource()
{
	m_Resource->Release();
	if (m_IntermediateResource)
	{
		m_IntermediateResource->Release();
	}
}

void DX12Resource::UpdateBufferResource(
	ID3D12Device* inDevice,
	ID3D12GraphicsCommandList2* inCommandList,
	size_t inBufferSize/* = 0*/, const void* inBufferData/* = nullptr*/)
{
	// Create an committed resource for the upload.
	if (inBufferData)
	{
		D3D12_HEAP_PROPERTIES	heap_properties	= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC		resource_desc	= CD3DX12_RESOURCE_DESC::Buffer(inBufferSize);

		// Create upload buffer on the CPU
		ThrowIfFailed(inDevice->CreateCommittedResource(
			&heap_properties,
			D3D12_HEAP_FLAG_NONE,
			&resource_desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_IntermediateResource)));

		D3D12_SUBRESOURCE_DATA subresource_data = {};
		subresource_data.pData		= inBufferData;
		subresource_data.RowPitch	= inBufferSize;
		subresource_data.SlicePitch	= subresource_data.RowPitch;

		UpdateSubresources(inCommandList,
						   m_Resource, m_IntermediateResource,
						   0, 0, 1, &subresource_data);
	}
}

DX12VertexBuffer::DX12VertexBuffer(
	ID3D12Device* inDevice,
	ID3D12GraphicsCommandList2* inCommandList,
	size_t inBufferSize, const void* inBufferData, ui32 inStride,
	D3D12_RESOURCE_FLAGS inFlags/* = D3D12_RESOURCE_FLAG_NONE*/)
	: DX12Resource(inDevice, inCommandList, inBufferSize, inBufferData, inFlags)
{
	m_VertexBufferView.BufferLocation	= m_Resource->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes		= (ui32) inBufferSize;
	m_VertexBufferView.StrideInBytes	= inStride;

	const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_Resource,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	inCommandList->ResourceBarrier(1, &barrier);
}

void DX12VertexBuffer::SetVertexBuffer(ID3D12GraphicsCommandList2* inCommandList, ui32 inStartSlot, ui32 inNumViews) const
{
	inCommandList->IASetVertexBuffers(inStartSlot, inNumViews, &m_VertexBufferView);
}

DX12IndexBuffer::DX12IndexBuffer(
	ID3D12Device* inDevice,
	ID3D12GraphicsCommandList2* inCommandList,
	size_t inBufferSize, const void* inBufferData,
	D3D12_RESOURCE_FLAGS inFlags/* = D3D12_RESOURCE_FLAG_NONE*/)
	: DX12Resource(inDevice, inCommandList, inBufferSize, inBufferData, inFlags)
{
	m_IndexBufferView.BufferLocation	= m_Resource->GetGPUVirtualAddress();
	m_IndexBufferView.Format			= DXGI_FORMAT_R16_UINT;
	m_IndexBufferView.SizeInBytes		= (ui32) inBufferSize;

	const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_Resource,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

	inCommandList->ResourceBarrier(1, &barrier);
}

void DX12IndexBuffer::SetIndexBuffer(ID3D12GraphicsCommandList2* inCommandList) const
{
	inCommandList->IASetIndexBuffer(&m_IndexBufferView);
}