#pragma once

#include "datatypes.h"

#include <d3d12.h>
#include <dxgi1_6.h>

#include "dx12/dx12fence.h"

#define NUM_FRAMES 3

class DX12Device
{
protected:
	// Use WARP adapter
	bool m_UseWarp = false;
	bool m_VSync = true;
	bool m_TearingSupported = false;

	// DirectX 12 Objects
	ID3D12Device2* m_Device;
	ID3D12CommandQueue* m_CommandQueue;
	IDXGISwapChain4* m_SwapChain;
	ID3D12Resource* m_BackBuffers[NUM_FRAMES];
	ID3D12GraphicsCommandList* m_CommandList;
	ID3D12CommandAllocator* m_CommandAllocators[NUM_FRAMES];
	ID3D12DescriptorHeap* m_RTVDescriptorHeap;
	ui32 m_RTVDescriptorSize;
	ui32 m_CurrentBackBufferIndex;

	DX12Fence* m_Fence;
	ui64 m_FrameFenceValues[NUM_FRAMES] = {};

	bool m_IsInitialized = false;

public:
	DX12Device();
	~DX12Device();
	void Init(HWND windowHandle, ui32 clientWidth, ui32 clientHeight);
	void Flush();
	void Present();
	void TempRendering();
	void UpdateRenderTargetViews(ui32 clientWidth, ui32 clientHeight);

protected:
	ID3D12Device2*				CreateDevice(IDXGIAdapter4* adapter);
	IDXGISwapChain4*			CreateSwapChain(HWND hWnd, ID3D12CommandQueue* commandQueue, ui32 width, ui32 height, ui32 bufferCount);
	ID3D12CommandQueue*			CreateCommandQueue(ID3D12Device2* device, D3D12_COMMAND_LIST_TYPE type);
	ID3D12DescriptorHeap*		CreateDescriptorHeap(ID3D12Device2* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
	ID3D12CommandAllocator*		CreateCommandAllocator(ID3D12Device2* device, D3D12_COMMAND_LIST_TYPE type);
	ID3D12GraphicsCommandList*	DX12Device::CreateCommandList(ID3D12Device2* device, ID3D12CommandAllocator* commandAllocator, D3D12_COMMAND_LIST_TYPE type);
	
	void EnableDebugLayer();
	bool CheckTearingSupport();
	IDXGIAdapter4* GetAdapter(bool useWarp);

public:
	inline bool GetIsInitialized() const
	{
		return m_IsInitialized;
	}
};