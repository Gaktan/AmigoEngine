#pragma once

#include "DX12/DX12Includes.h"

class DX12CommandQueue;
class DX12SwapChain;
class DX12DescriptorHeap;
class DX12FreeListDescriptorHeap;

class DX12Device final
{
public:
	DX12Device();
	~DX12Device();

private:
	ID3D12Device2* CreateDevice(IDXGIAdapter4& inAdapter);

	void EnableGPUBasedValidation();
	void EnableDebugLayer();

	IDXGIAdapter4* GetAdapter(bool inUseWarp);

public:
	void Init(HWND inWindowHandle, uint32 inWidth, uint32 inHeight);
	void Release();
	void Flush();
	void Present(ID3D12GraphicsCommandList2& inCommandList);

	DX12CommandQueue&		GetCommandQueue(D3D12_COMMAND_LIST_TYPE inType) const;
	DX12DescriptorHeap&		GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE inType) const;

	inline ID3D12Device2&	GetD3DDevice() const		{ return *m_D3DDevice; }
	inline DX12SwapChain&	GetSwapChain() const		{ return *m_SwapChain; }
	inline uint64			GetFrameID() const			{ return m_FrameID; }
	inline bool				IsInitialized() const		{ return m_IsInitialized; }


private:
	// Command queues
	DX12CommandQueue*	m_DirectCommandQueue;
	DX12CommandQueue*	m_ComputeCommandQueue;
	DX12CommandQueue*	m_CopyCommandQueue;

	// Descriptor heaps
	DX12FreeListDescriptorHeap*		m_RTVDescriptorHeap;
	DX12FreeListDescriptorHeap*		m_DSVDescriptorHeap;
	DX12FreeListDescriptorHeap*		m_SRVDescriptorHeap;

	ID3D12Device2*		m_D3DDevice;
	DX12SwapChain*		m_SwapChain;

	uint64				m_FrameID		= 0;

	bool				m_UseWarp		= false;
	bool				m_IsInitialized	= false;
};

extern DX12Device g_RenderingDevice;