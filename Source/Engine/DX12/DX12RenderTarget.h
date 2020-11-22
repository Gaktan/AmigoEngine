#pragma once

#include "DX12/DX12Device.h"
#include "DX12/DX12Resource.h"

// TODO: extend from texture
class DX12RenderTarget : public DX12Resource
{
protected:
	uint32			m_Width			= 0;
	uint32			m_Height		= 0;
	DXGI_FORMAT		m_Format		= DXGI_FORMAT_UNKNOWN;
	Vec4			m_ClearValue	= Vec4(0.0f);

	D3D12_CPU_DESCRIPTOR_HANDLE	m_RTVDescriptorHandle = { 0 };

public:
	virtual ~DX12RenderTarget()
	{
	}

	void InitAsRenderTarget(
		uint32 inWidth, uint32 inHeight, DXGI_FORMAT inFormat = DXGI_FORMAT_UNKNOWN,
		Vec4 inClearValue = Vec4(0.0f));

	void InitFromResource(
		ID3D12Resource* inResource,
		DXGI_FORMAT inFormat = DXGI_FORMAT_UNKNOWN,
		Vec4 inClearValue = Vec4(0.0f));

	virtual void ReleaseResources() override;

	void ClearBuffer(ID3D12GraphicsCommandList2* inCommandList) const;

	D3D12_CPU_DESCRIPTOR_HANDLE		GetCPUDescriptorHandle() const;
	DXGI_FORMAT						GetFormat() const;
};

// TODO: extend from texture
class DX12DepthBuffer : public DX12Resource
{
protected:
	uint32			m_Width				= 0;
	uint32			m_Height			= 0;
	DXGI_FORMAT		m_Format			= DXGI_FORMAT_UNKNOWN;
	float			m_ClearValue		= 0.0f;
	uint8			m_StencilClearValue = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE	m_DSVDescriptorHandle = { 0 };

public:
	virtual ~DX12DepthBuffer()
	{
	}

	void InitAsDepthStencilBuffer(
		uint32 inWidth, uint32 inHeight,
		float inClearValue = 1.0f, uint8 inStencilClearValue = 0,
		DXGI_FORMAT inDepthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT);

	virtual void ReleaseResources() override;

	void ClearBuffer(ID3D12GraphicsCommandList2* inCommandList) const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() const;
};