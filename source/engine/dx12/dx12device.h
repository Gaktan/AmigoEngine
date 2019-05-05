#pragma once

#define NUM_FRAMES 3

#include "datatypes.h"
#include "dx12/dx12fence.h"
#include "dx12/dx12commandqueue.h"

#include <d3d12.h>
#include <dxgi1_6.h>


class DX12Device
{
protected:
public: //Temp hack
	// Use WARP adapter
	bool m_UseWarp = false;
	bool m_VSync = true;
	bool m_TearingSupported = false;

	// DirectX 12 Objects
	ID3D12Device2*					m_Device;
	IDXGISwapChain4*				m_SwapChain;
	ID3D12Resource*					m_BackBuffers[NUM_FRAMES];
	ID3D12DescriptorHeap*			m_RTVDescriptorHeap;
	ui32							m_RTVDescriptorSize;
	ui32							m_CurrentBackBufferIndex;

	ui64							m_FrameFenceValues[NUM_FRAMES] = {};

	bool							m_IsInitialized = false;

	DX12CommandQueue*				m_DirectCommandQueue;
	DX12CommandQueue*				m_ComputeCommandQueue;
	DX12CommandQueue*				m_CopyCommandQueue;

public:
	DX12Device();
	~DX12Device();
	void Init(HWND windowHandle, ui32 clientWidth, ui32 clientHeight);
	void Flush();
	void Present(ID3D12GraphicsCommandList2* commandList);
	void ClearBackBuffer(ID3D12GraphicsCommandList2* commandList);
	void UpdateRenderTargetViews(ui32 clientWidth, ui32 clientHeight);

	DX12CommandQueue* GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const;

protected:
	ID3D12Device2*				CreateDevice(IDXGIAdapter4* adapter);
	IDXGISwapChain4*			CreateSwapChain(HWND hWnd, DX12CommandQueue* commandQueue, ui32 width, ui32 height, ui32 bufferCount);
	ID3D12DescriptorHeap*		CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
	
	void EnableGPUBasedValidation();
	void EnableDebugLayer();

	bool CheckTearingSupport();
	IDXGIAdapter4* GetAdapter(bool useWarp);

public:
	inline bool GetIsInitialized() const
	{
		return m_IsInitialized;
	}
};