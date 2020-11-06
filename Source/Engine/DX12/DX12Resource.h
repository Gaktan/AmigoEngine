#pragma once

#include "DX12/DX12Includes.h"
#include <string>

class DX12Resource
{
protected:
	ID3D12Resource* m_Resource				= nullptr;
	ID3D12Resource* m_IntermediateResource	= nullptr;

public:
	virtual ~DX12Resource();

	virtual void InitAsResource(
		ID3D12GraphicsCommandList2* inCommandList,
		size_t inBufferSize = 0, const void* inBufferData = nullptr,
		D3D12_RESOURCE_FLAGS inFlags = D3D12_RESOURCE_FLAG_NONE);

	virtual void ReleaseResources();

	virtual void UpdateBufferResource(
		ID3D12GraphicsCommandList2* inCommandList,
		size_t inBufferSize = 0, const void* inBufferData = nullptr);

	ID3D12Resource* GetResource() const;

protected:
	static void SetResourceName(ID3D12Resource* inResource, const std::string& inName);
};

class DX12VertexBuffer : public DX12Resource
{
protected:
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;

public:
	virtual ~DX12VertexBuffer()
	{
	}

	void InitAsVertexBuffer(
		ID3D12GraphicsCommandList2* inCommandList,
		size_t inBufferSize, const void* inBufferData, uint32 inStride,
		D3D12_RESOURCE_FLAGS inFlags = D3D12_RESOURCE_FLAG_NONE);

	void SetVertexBuffer(ID3D12GraphicsCommandList2* inCommandList, uint32 inStartSlot) const;
};

class DX12IndexBuffer : public DX12Resource
{
protected:
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

public:
	virtual ~DX12IndexBuffer()
	{
	}

	void InitAsIndexBuffer(
		ID3D12GraphicsCommandList2* inCommandList,
		size_t inBufferSize, const void* inBufferData,
		D3D12_RESOURCE_FLAGS inFlags = D3D12_RESOURCE_FLAG_NONE);

	void SetIndexBuffer(ID3D12GraphicsCommandList2* inCommandList) const;
};

class DX12ConstantBuffer : public DX12Resource
{
public:
	virtual ~DX12ConstantBuffer()
	{
	}

	void InitAsConstantBuffer(size_t inBufferSize);

	virtual void UpdateBufferResource(
		ID3D12GraphicsCommandList2* inCommandList,
		size_t inBufferSize = 0, const void* inBufferData = nullptr) override;

	void SetConstantBuffer(ID3D12GraphicsCommandList2* inCommandList, uint32 inRootParameterIndex) const;
};