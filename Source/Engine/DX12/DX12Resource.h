#pragma once

#include "DX12/DX12Includes.h"
#include <string>

class DX12Resource
{
public:
	DX12Resource() = default;
	virtual ~DX12Resource() = default;

protected:
	virtual void InitAsResource(
		ID3D12GraphicsCommandList2* inCommandList,
		size_t inBufferSize = 0, const void* inBufferData = nullptr,
		D3D12_RESOURCE_FLAGS inFlags = D3D12_RESOURCE_FLAG_NONE);

	virtual void UpdateBufferResource(
		ID3D12GraphicsCommandList2* inCommandList,
		size_t inBufferSize = 0, const void* inBufferData = nullptr);

	virtual void OnReleased()						{}

public:
	inline ID3D12Resource*	GetResource() const		{ return m_Resource; }
	void					Release();

protected:
	static void SetResourceName(ID3D12Resource* inResource, const std::string& inName);

protected:
	ID3D12Resource* m_Resource				= nullptr;
	ID3D12Resource* m_IntermediateResource	= nullptr;
};

class DX12VertexBuffer final : public DX12Resource
{
public:
	using DX12Resource::DX12Resource;

public:
	void InitAsVertexBuffer(
		ID3D12GraphicsCommandList2* inCommandList,
		size_t inBufferSize, const void* inBufferData, uint32 inStride,
		D3D12_RESOURCE_FLAGS inFlags = D3D12_RESOURCE_FLAG_NONE);

	void SetVertexBuffer(ID3D12GraphicsCommandList2* inCommandList, uint32 inStartSlot) const;

private:
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
};

class DX12IndexBuffer final : public DX12Resource
{
public:
	using DX12Resource::DX12Resource;

public:
	void InitAsIndexBuffer(
		ID3D12GraphicsCommandList2* inCommandList,
		size_t inBufferSize, const void* inBufferData,
		D3D12_RESOURCE_FLAGS inFlags = D3D12_RESOURCE_FLAG_NONE);

	void SetIndexBuffer(ID3D12GraphicsCommandList2* inCommandList) const;

protected:
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
};

class DX12ConstantBuffer final : public DX12Resource
{
public:
	using DX12Resource::DX12Resource;

	virtual void UpdateBufferResource(
		ID3D12GraphicsCommandList2* inCommandList,
		size_t inBufferSize = 0, const void* inBufferData = nullptr) override;

	void SetConstantBuffer(ID3D12GraphicsCommandList2* inCommandList, uint32 inRootParameterIndex) const;

	void InitAsConstantBuffer(size_t inBufferSize);
};