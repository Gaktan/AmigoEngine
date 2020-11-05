#pragma once

#include "DX12/DX12Device.h"

class DX12DepthBuffer;
class DX12RenderTarget;

class GBuffer
{
protected:
	DX12DepthBuffer*	m_DepthBuffer		= nullptr;
	DX12RenderTarget*	m_RenderTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
	uint32				m_NumRenderTargets	= 0;

	uint32				m_Width				= 0;
	uint32				m_Height			= 0;

public:
	GBuffer();
	~GBuffer();

	void ReleaseResources();
	void AllocateResources(DX12Device& inDevice, uint32 inTargetWidth, uint32 inTargetHeight);
	
	void Set(ID3D12GraphicsCommandList2* inCommandList) const;
	void ClearDepthBuffer(ID3D12GraphicsCommandList2* inCommandList) const;
	void ClearRenderTargets(ID3D12GraphicsCommandList2* inCommandList) const;
};