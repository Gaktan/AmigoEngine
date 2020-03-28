#pragma once

constexpr uint32 NUM_BUFFERED_FRAMES = 3;

#include <d3d12.h>
#include <dxgi1_6.h>

#include "DX12/DX12Device.h"
#include "DX12/DX12CommandQueue.h"
#include "DX12/DX12RenderTarget.h"

class DX12SwapChain
{
protected:
	IDXGISwapChain4*		m_D3DSwapChain;

	DX12RenderTarget*		m_BackBuffers[NUM_BUFFERED_FRAMES];

	uint32					m_CurrentBackBufferIndex;
	uint64					m_FrameFenceValues[NUM_BUFFERED_FRAMES] = {};

	bool					m_VSync				= true;
	bool					m_TearingSupported	= false;

public:
	DX12SwapChain(DX12Device& inDevice, HWND inHandle, const DX12CommandQueue& inCommandQueue, uint32 inWidth, uint32 inHeight);
	virtual ~DX12SwapChain();

	void UpdateRenderTargetViews(DX12Device& inDevice, uint32 inClientWidth, uint32 inClientHeight, bool inFirstCall = false);
	void ClearBackBuffer(ID3D12GraphicsCommandList2* inCommandList) const;
	void Present(ID3D12GraphicsCommandList2* commandList, DX12CommandQueue* commandQueue);

	void SetRenderTarget(ID3D12GraphicsCommandList2* inCommandList, const DX12DepthRenderTarget* inDepthBuffer);

	uint32 GetCurrentBackBufferIndex();
};