#include "engine_precomp.h"
#include "dx12resource.h"

#include "D3dx12.h"

#include <cassert>
#include <exception>

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

DX12Resource::DX12Resource()
	: m_IntermediateResource(nullptr)
{
}

DX12Resource::DX12Resource(
	ID3D12Device* device,
	ID3D12GraphicsCommandList2* commandList,
	size_t bufferSize/* = 0*/, const void* bufferData/* = nullptr*/,
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

	UpdateBufferResource(device, commandList, bufferSize, bufferData);
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
	ID3D12Device* device,
	ID3D12GraphicsCommandList2* commandList,
	size_t bufferSize/* = 0*/, const void* bufferData/* = nullptr*/)
{
	// Create an committed resource for the upload.
	if (bufferData)
	{
		// Create upload buffer on the CPU
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_IntermediateResource)));

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = bufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		UpdateSubresources(commandList,
						   m_Resource, m_IntermediateResource,
						   0, 0, 1, &subresourceData);
	}
}

DX12VertexBuffer::DX12VertexBuffer(
	ID3D12Device* device,
	ID3D12GraphicsCommandList2* commandList,
	size_t bufferSize, const void* bufferData, ui32 stride,
	D3D12_RESOURCE_FLAGS flags/* = D3D12_RESOURCE_FLAG_NONE*/)
	: DX12Resource(device, commandList, bufferSize, bufferData, flags)
{
	m_VertexBufferView.BufferLocation = m_Resource->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes = bufferSize;
	m_VertexBufferView.StrideInBytes = stride;

	const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_Resource,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	
	commandList->ResourceBarrier(1, &barrier);
}

void DX12VertexBuffer::SetVertexBuffer(ID3D12GraphicsCommandList2* commandList, ui32 startSlot, ui32 numViews) const
{
	commandList->IASetVertexBuffers(startSlot, numViews, &m_VertexBufferView);
}

DX12IndexBuffer::DX12IndexBuffer(
	ID3D12Device* device,
	ID3D12GraphicsCommandList2* commandList,
	size_t bufferSize, const void* bufferData,
	D3D12_RESOURCE_FLAGS flags/* = D3D12_RESOURCE_FLAG_NONE*/)
	: DX12Resource(device, commandList, bufferSize, bufferData, flags)
{
	m_IndexBufferView.BufferLocation = m_Resource->GetGPUVirtualAddress();
	m_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	m_IndexBufferView.SizeInBytes = bufferSize;

	const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_Resource,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

	commandList->ResourceBarrier(1, &barrier);
}

void DX12IndexBuffer::SetIndexBuffer(ID3D12GraphicsCommandList2* commandList) const
{
	commandList->IASetIndexBuffer(&m_IndexBufferView);
}