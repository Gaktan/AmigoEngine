#include "Engine.h"
#include "DX12Resource.h"

#include "D3dx12.h"

void DX12Resource::InitAsResource(
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
		if (m_IntermediateResource == nullptr)
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
		}

		D3D12_SUBRESOURCE_DATA subresource_data = {};
		subresource_data.pData		= inBufferData;
		subresource_data.RowPitch	= inBufferSize;
		subresource_data.SlicePitch	= subresource_data.RowPitch;

		UpdateSubresources(inCommandList,
						   m_Resource, m_IntermediateResource,
						   0, 0, 1, &subresource_data);
	}
}

void DX12VertexBuffer::InitAsVertexBuffer(
	ID3D12Device* inDevice,
	ID3D12GraphicsCommandList2* inCommandList,
	size_t inBufferSize, const void* inBufferData, uint32 inStride,
	D3D12_RESOURCE_FLAGS inFlags/* = D3D12_RESOURCE_FLAG_NONE*/)
{
	DX12Resource::InitAsResource(inDevice, inCommandList, inBufferSize, inBufferData, inFlags);

	m_VertexBufferView.BufferLocation	= m_Resource->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes		= (uint32) inBufferSize;
	m_VertexBufferView.StrideInBytes	= inStride;

	const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_Resource,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	inCommandList->ResourceBarrier(1, &barrier);
}

void DX12VertexBuffer::SetVertexBuffer(ID3D12GraphicsCommandList2* inCommandList, uint32 inStartSlot) const
{
	inCommandList->IASetVertexBuffers(inStartSlot, 1, &m_VertexBufferView);
}

void DX12IndexBuffer::InitAsIndexBuffer(
	ID3D12Device* inDevice,
	ID3D12GraphicsCommandList2* inCommandList,
	size_t inBufferSize, const void* inBufferData,
	D3D12_RESOURCE_FLAGS inFlags/* = D3D12_RESOURCE_FLAG_NONE*/)
{
	DX12Resource::InitAsResource(inDevice, inCommandList, inBufferSize, inBufferData, inFlags);

	m_IndexBufferView.BufferLocation	= m_Resource->GetGPUVirtualAddress();
	m_IndexBufferView.Format			= DXGI_FORMAT_R16_UINT;
	m_IndexBufferView.SizeInBytes		= (uint32) inBufferSize;

	const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_Resource,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

	inCommandList->ResourceBarrier(1, &barrier);
}

void DX12IndexBuffer::SetIndexBuffer(ID3D12GraphicsCommandList2* inCommandList) const
{
	inCommandList->IASetIndexBuffer(&m_IndexBufferView);
}

void DX12ConstantBuffer::InitAsConstantBuffer(
	ID3D12Device* inDevice,
	ID3D12GraphicsCommandList2* inCommandList,
	size_t inBufferSize, const void* inBufferData,
	D3D12_RESOURCE_FLAGS inFlags/* = D3D12_RESOURCE_FLAG_NONE*/)
{
	D3D12_HEAP_PROPERTIES	heap_properties	= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC		resource_desc	= CD3DX12_RESOURCE_DESC::Buffer(inBufferSize);

	// These will remain in upload heap because we use them only once per frame
	ThrowIfFailed(inDevice->CreateCommittedResource(
		&heap_properties,
		D3D12_HEAP_FLAG_NONE,
		&resource_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_Resource)));
}

void DX12ConstantBuffer::UpdateBufferResource(
	ID3D12Device* inDevice,
	ID3D12GraphicsCommandList2* inCommandList,
	size_t inBufferSize/* = 0*/, const void* inBufferData/* = nullptr*/)
{
	// Perform Map/Unmap with the new data
	if (inBufferData)
	{
		void* p;
		m_Resource->Map(0, nullptr, &p);
		::memcpy(p, inBufferData, inBufferSize);
		m_Resource->Unmap(0, nullptr);
	}
}

void DX12ConstantBuffer::SetConstantBuffer(ID3D12GraphicsCommandList2* inCommandList, uint32 inRootParameterIndex) const
{
	inCommandList->SetGraphicsRootConstantBufferView(inRootParameterIndex, m_Resource->GetGPUVirtualAddress());
}