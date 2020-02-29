#pragma once

#include <d3d12.h>

class DX12Resource
{
protected:
	ID3D12Resource* m_Resource				= nullptr;
	ID3D12Resource* m_IntermediateResource	= nullptr;

public:
	DX12Resource(
		ID3D12Device* inDevice,
		ID3D12GraphicsCommandList2* inCommandList,
		size_t inBufferSize = 0, const void* inBufferData = nullptr,
		D3D12_RESOURCE_FLAGS inFlags = D3D12_RESOURCE_FLAG_NONE);

	DX12Resource()
	{
	}

	virtual ~DX12Resource();

	virtual void UpdateBufferResource(
		ID3D12Device* inDevice,
		ID3D12GraphicsCommandList2* inCommandList,
		size_t inBufferSize = 0, const void* inBufferData = nullptr);
};

class DX12VertexBuffer : public DX12Resource
{
protected:
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;

public:
	DX12VertexBuffer(
		ID3D12Device* inDevice,
		ID3D12GraphicsCommandList2* inCommandList,
		size_t inBufferSize, const void* inBufferData, uint32 inStride,
		D3D12_RESOURCE_FLAGS inFlags = D3D12_RESOURCE_FLAG_NONE);

	virtual ~DX12VertexBuffer()
	{
	}

	void SetVertexBuffer(ID3D12GraphicsCommandList2* inCommandList, uint32 inStartSlot, uint32 inNumViews) const;
};

class DX12IndexBuffer : public DX12Resource
{
protected:
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

public:
	DX12IndexBuffer(
		ID3D12Device* inDevice,
		ID3D12GraphicsCommandList2* inCommandList,
		size_t inBufferSize, const void* inBufferData,
		D3D12_RESOURCE_FLAGS inFlags = D3D12_RESOURCE_FLAG_NONE);

	virtual ~DX12IndexBuffer()
	{
	}

	void SetIndexBuffer(ID3D12GraphicsCommandList2* inCommandList) const;
};

class DX12ConstantBuffer : public DX12Resource
{
public:
	DX12ConstantBuffer(
		ID3D12Device* inDevice,
		ID3D12GraphicsCommandList2* inCommandList,
		size_t inBufferSize, const void* inBufferData,
		D3D12_RESOURCE_FLAGS inFlags = D3D12_RESOURCE_FLAG_NONE);

	virtual ~DX12ConstantBuffer()
	{
	}

	virtual void UpdateBufferResource(
		ID3D12Device* inDevice,
		ID3D12GraphicsCommandList2* inCommandList,
		size_t inBufferSize = 0, const void* inBufferData = nullptr) override;

	void SetConstantBuffer(ID3D12GraphicsCommandList2* inCommandList, uint32 inRootParameterIndex) const;
};