#pragma once

#include "DX12/DX12Includes.h"
#include "DX12/DX12CommandQueue.h"
#include "DX12/DX12RenderTarget.h"

class DX12SwapChain final
{
	friend class DX12Device;
private:
	DX12SwapChain(HWND inHandle, const DX12CommandQueue& inCommandQueue, uint32 inWidth, uint32 inHeight);
	~DX12SwapChain();

public:
	void UpdateRenderTargetViews(uint32 inClientWidth, uint32 inClientHeight, bool inFirstCall = false);
	void ClearBackBuffer(ID3D12GraphicsCommandList2* inCommandList) const;
	void Present(ID3D12GraphicsCommandList2* commandList, DX12CommandQueue* commandQueue);

	void SetRenderTarget(ID3D12GraphicsCommandList2* inCommandList);

private:
	enum
	{
		NUM_BUFFERED_FRAMES = 3
	};

	IDXGISwapChain4*	m_D3DSwapChain;

	DX12RenderTarget*	m_BackBuffers[NUM_BUFFERED_FRAMES];

	uint32				m_CurrentBackBufferIndex;
	uint64				m_FrameFenceValues[NUM_BUFFERED_FRAMES] = {};

	bool				m_VSync				= true;
	bool				m_TearingSupported	= false;
};