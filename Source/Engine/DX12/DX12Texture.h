#pragma once

#include "DX12/DX12Resource.h"

class DX12Texture final : public DX12Resource
{
public:
	using DX12Resource::DX12Resource;

private:
	virtual void UpdateBufferResource(
		ID3D12GraphicsCommandList2& inCommandList,
		size_t inBufferSize = 0, const void* inBufferData = nullptr) override;

	void OnReleased() override;

public:
	void InitAsTexture(
		ID3D12GraphicsCommandList2& inCommandList,
		uint32 inWidth, uint32 inHeight, DXGI_FORMAT inFormat,
		const void* inBufferData);

	inline D3D12_CPU_DESCRIPTOR_HANDLE		GetCPUHandle() const { return m_CPUHandle; }

private:
	uint32							m_Width;
	uint32							m_Height;
	DXGI_FORMAT						m_Format;
	D3D12_CPU_DESCRIPTOR_HANDLE		m_CPUHandle		= { 0 };
};