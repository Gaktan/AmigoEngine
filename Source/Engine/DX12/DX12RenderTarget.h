#pragma once

#include "DX12/DX12Device.h"
#include "DX12/DX12Resource.h"

// TODO: extend from texture
class DX12RenderTarget final : public DX12Resource
{
public:
	using DX12Resource::DX12Resource;

private:
	void OnReleased() override;

public:
	void InitAsRenderTarget(
		uint32 inWidth, uint32 inHeight, DXGI_FORMAT inFormat = DXGI_FORMAT_UNKNOWN,
		Vec4 inClearValue = Vec4(0.0f));

	void InitFromResource(
		ID3D12Resource& inResource,
		DXGI_FORMAT inFormat = DXGI_FORMAT_UNKNOWN,
		Vec4 inClearValue = Vec4(0.0f));

	void ClearBuffer(ID3D12GraphicsCommandList2& inCommandList) const;

	inline D3D12_CPU_DESCRIPTOR_HANDLE		GetCPUDescriptorHandle() const	{ return m_RTVDescriptorHandle; }
	inline DXGI_FORMAT						GetFormat() const				{ return m_Format; }

private:
	uint32			m_Width			= 0;
	uint32			m_Height		= 0;
	DXGI_FORMAT		m_Format		= DXGI_FORMAT_UNKNOWN;
	Vec4			m_ClearValue	= Vec4(0.0f);

	D3D12_CPU_DESCRIPTOR_HANDLE	m_RTVDescriptorHandle = { 0 };
};

// TODO: extend from texture
class DX12DepthBuffer final : public DX12Resource
{
public:
	using DX12Resource::DX12Resource;

private:
	void OnReleased() override;

public:
	void InitAsDepthStencilBuffer(
		uint32 inWidth, uint32 inHeight,
		float inClearValue = 1.0f, uint8 inStencilClearValue = 0,
		DXGI_FORMAT inDepthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT);

	void ClearBuffer(ID3D12GraphicsCommandList2& inCommandList) const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() const		{ return m_DSVDescriptorHandle; }

private:
	uint32			m_Width				= 0;
	uint32			m_Height			= 0;
	DXGI_FORMAT		m_Format			= DXGI_FORMAT_UNKNOWN;
	float			m_ClearValue		= 0.0f;
	uint8			m_StencilClearValue = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE	m_DSVDescriptorHandle = { 0 };
};