#pragma once

#include "datatypes.h"

#include "dx12resource.h"

// TODO!
#if 0
class DX12RenderTarget : public DX12Resource
{
public:
	DX12RenderTarget(
		ID3D12Device* device,
		ID3D12GraphicsCommandList2* commandList,
		ui32 width, ui32 height, DXGI_FORMAT format,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	DX12RenderTarget();

	virtual ~DX12RenderTarget();
};
#endif

class DX12DepthRenderTarget : public DX12Resource //DX12RenderTarget
{
protected:
	ID3D12DescriptorHeap*	m_DSVHeap;
	float					m_ClearValue;
	DXGI_FORMAT				m_Format;

public:
	DX12DepthRenderTarget(
		ID3D12Device* device,
		ui32 width, ui32 height, float clearValue = 1.0f, DXGI_FORMAT depthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT);

	virtual ~DX12DepthRenderTarget();

	void ClearDepth(ID3D12GraphicsCommandList2* commandList);

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle();
	DXGI_FORMAT					GetFormat();
};