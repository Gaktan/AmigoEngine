#pragma once

#include "datatypes.h"

#include <d3d12.h>
#include <dxgi1_6.h>

#include "dx12/dx12commandqueue.h"
#include "dx12/dx12rendertarget.h"

constexpr ui32 NUM_BUFFERED_FRAMES = 3;

class DX12SwapChain
{
protected:
	IDXGISwapChain4*		m_SwapChain;

	ID3D12Resource*			m_BackBuffers[NUM_BUFFERED_FRAMES];
	ID3D12DescriptorHeap*	m_RTVDescriptorHeap;
	ui32					m_RTVDescriptorSize;

	ui32					m_CurrentBackBufferIndex;
	ui64					m_FrameFenceValues[NUM_BUFFERED_FRAMES] = {};

	bool					m_VSync;
	bool					m_TearingSupported;

public:
	DX12SwapChain(DX12Device& device, HWND hWnd, DX12CommandQueue* commandQueue, ui32 width, ui32 height);
	virtual ~DX12SwapChain();

	void UpdateRenderTargetViews(DX12Device& device, ui32 clientWidth, ui32 clientHeight, bool firstCall = false);
	void ClearBackBuffer(ID3D12GraphicsCommandList2* commandList);
	void Present(ID3D12GraphicsCommandList2* commandList, DX12CommandQueue* commandQueue);

	void SetRenderTarget(ID3D12GraphicsCommandList2* commandList, DX12DepthRenderTarget* depthBuffer);
};