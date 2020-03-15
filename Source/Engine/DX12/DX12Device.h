#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>

class DX12CommandQueue;
class DX12SwapChain;
class DX12DescriptorHeap;
class DX12FreeListDescriptorHeap;

class DX12Device
{
public: //Temp hack
	ID3D12Device2*			m_Device;
	DX12SwapChain*			m_SwapChain;

protected:
	// Use WARP adapter
	bool					m_UseWarp = false;

	// Command queues
	DX12CommandQueue*		m_DirectCommandQueue;
	DX12CommandQueue*		m_ComputeCommandQueue;
	DX12CommandQueue*		m_CopyCommandQueue;

	// Descriptor heaps
	DX12DescriptorHeap*				m_RTVDescriptorHeap;
	DX12DescriptorHeap*				m_DSVDescriptorHeap;
	DX12FreeListDescriptorHeap*		m_SRVDescriptorHeap;

public:
	DX12Device();
	virtual ~DX12Device();
	void Init(HWND inWindowHandle, uint32 inWidth, uint32 inHeight);
	void Flush();
	void Present(ID3D12GraphicsCommandList2* inCommandList);

	void ResestDescriptorHeaps();

	DX12CommandQueue*	GetCommandQueue(D3D12_COMMAND_LIST_TYPE inType) const;
	DX12DescriptorHeap*	GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE inType) const;

protected:
	ID3D12Device2* CreateDevice(IDXGIAdapter4* inAdapter);

	void EnableGPUBasedValidation();
	void EnableDebugLayer();

	IDXGIAdapter4* GetAdapter(bool inUseWarp);
};