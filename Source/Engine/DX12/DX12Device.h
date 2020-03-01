#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>

class DX12CommandQueue;
class DX12SwapChain;

struct DX12DescriptorHeap
{
	ID3D12DescriptorHeap*	m_DescriptorHeap;
	uint32					m_DescriptorIncrementSize;
};

class DX12Device
{
protected:
public: //Temp hack
	ID3D12Device2*			m_Device;
	DX12SwapChain*			m_SwapChain;

protected:
	// Use WARP adapter
	bool					m_UseWarp = false;

	DX12CommandQueue*		m_DirectCommandQueue;
	DX12CommandQueue*		m_ComputeCommandQueue;
	DX12CommandQueue*		m_CopyCommandQueue;

	// RTV Descriptor Heap
	DX12DescriptorHeap		m_RTVDescriptorHeap;

	// DSV Descriptor Heap
	DX12DescriptorHeap		m_DSVDescriptorHeap;

public:
	DX12Device();
	~DX12Device();
	void Init(HWND inWindowHandle, uint32 inWidth, uint32 inHeight);
	void Flush();
	void Present(ID3D12GraphicsCommandList2* inCommandList);

	DX12CommandQueue*	GetCommandQueue(D3D12_COMMAND_LIST_TYPE inType) const;
	DX12DescriptorHeap	GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE inType) const;

protected:
	ID3D12Device2* CreateDevice(IDXGIAdapter4* inAdapter);
	DX12DescriptorHeap CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE inHeapType);

	void EnableGPUBasedValidation();
	void EnableDebugLayer();

	IDXGIAdapter4* GetAdapter(bool inUseWarp);
};