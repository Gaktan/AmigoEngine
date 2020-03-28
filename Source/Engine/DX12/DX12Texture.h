#pragma once

#include "DX12/DX12Device.h"
#include "DX12/DX12Resource.h"
#include "Math/Vec4.h"

class DX12Texture : public DX12Resource
{
protected:
	uint32					m_Width;
	uint32					m_Height;
	DXGI_FORMAT				m_Format;
	DX12DescriptorHeap*		m_DescriptorHeap		= nullptr;
	uint32					m_DescriptorHeapIndex;

public:
	virtual ~DX12Texture();

	void InitAsTexture(
		DX12Device& inDevice,
		ID3D12GraphicsCommandList2* inCommandList,
		DX12DescriptorHeap* inDescriptorHeap,
		uint32 inWidth, uint32 inHeight, DXGI_FORMAT inFormat,
		const void* inBufferData);

	D3D12_CPU_DESCRIPTOR_HANDLE		GetCPUHandle() const;
	D3D12_GPU_DESCRIPTOR_HANDLE		GetGPUHandle() const;

protected:
	virtual void UpdateBufferResource(
		DX12Device& inDevice,
		ID3D12GraphicsCommandList2* inCommandList,
		size_t inBufferSize = 0, const void* inBufferData = nullptr) override;
};