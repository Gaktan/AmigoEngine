#pragma once

#include <d3d12.h>

class DX12Resource
{
protected:
	ID3D12Resource* m_Resource;
	ID3D12Resource* m_IntermediateResource;

public:
	DX12Resource(
		ID3D12Device* device,
		ID3D12GraphicsCommandList2* commandList,
		size_t bufferSize = 0, const void* bufferData = nullptr,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	DX12Resource();

	virtual ~DX12Resource();

	void UpdateBufferResource(
		ID3D12Device* device,
		ID3D12GraphicsCommandList2* commandList,
		size_t bufferSize = 0, const void* bufferData = nullptr);
};

class DX12VertexBuffer : public DX12Resource
{
protected:
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;

public:
	DX12VertexBuffer(
		ID3D12Device* device,
		ID3D12GraphicsCommandList2* commandList,
		size_t bufferSize, const void* bufferData, ui32 stride,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	virtual ~DX12VertexBuffer()
	{
	}

	void SetVertexBuffer(ID3D12GraphicsCommandList2* commandList, ui32 startSlot, ui32 numViews) const;
};

class DX12IndexBuffer : public DX12Resource
{
protected:
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

public:
	DX12IndexBuffer(
		ID3D12Device* device,
		ID3D12GraphicsCommandList2* commandList,
		size_t bufferSize, const void* bufferData,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	virtual ~DX12IndexBuffer()
	{
	}

	void SetIndexBuffer(ID3D12GraphicsCommandList2* commandList) const;
};