#pragma once

#include "DX12/DX12Device.h"
#include "DX12/DX12Resource.h"
#include "Math/Vec4.h"

class DX12RenderTarget : public DX12Resource
{
protected:
	Vec4			m_ClearValue	= 0.0f;
	DXGI_FORMAT		m_Format		= DXGI_FORMAT_UNKNOWN;

	D3D12_CPU_DESCRIPTOR_HANDLE	m_DescriptorHandle;

public:
	virtual ~DX12RenderTarget()
	{
	}

	void InitAsRenderTarget(
		DX12Device& inDevice,
		D3D12_CPU_DESCRIPTOR_HANDLE inDescriptorHandle,
		uint32 inWidth, uint32 inHeight, Vec4 inClearValue = 1.0f,
		DXGI_FORMAT inFormat = DXGI_FORMAT_UNKNOWN);

	void InitFromResource(
		DX12Device& inDevice,
		ID3D12Resource* inResource,
		D3D12_CPU_DESCRIPTOR_HANDLE inDescriptorHandle,
		Vec4 inClearValue = 1.0f,
		DXGI_FORMAT inFormat = DXGI_FORMAT_D24_UNORM_S8_UINT);

	virtual void ClearBuffer(ID3D12GraphicsCommandList2* inCommandList) const;

	D3D12_CPU_DESCRIPTOR_HANDLE		GetCPUDescriptorHandle() const;
	DXGI_FORMAT						GetFormat() const;
};

class DX12DepthRenderTarget : public DX12RenderTarget
{
public:
	virtual ~DX12DepthRenderTarget()
	{
	}

	void InitAsDepthStencilBuffer(
		DX12Device& inDevice,
		D3D12_CPU_DESCRIPTOR_HANDLE inDescriptorHandle,
		uint32 inWidth, uint32 inHeight,
		float inClearValue = 1.0f, uint8 inStencilClearValue = 0,
		DXGI_FORMAT inDepthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT);

	virtual void ClearBuffer(ID3D12GraphicsCommandList2* inCommandList) const override;
};