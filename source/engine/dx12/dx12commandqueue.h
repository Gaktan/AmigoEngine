#pragma once

#include "dx12/dx12device.h"
#include "dx12/dx12fence.h"
// For NUM_BUFFERED_FRAMES
#include "dx12/dx12swapchain.h"

#include <queue>

class DX12CommandQueue
{
protected:
	D3D12_COMMAND_LIST_TYPE		m_CommandListType;
	ID3D12CommandQueue*			m_CommandQueue;

	DX12Fence					m_Fence;
	ui32						m_CurrentIndex;

	struct CommandListEntry
	{
		ID3D12CommandAllocator*		m_CommandAllocator	= nullptr;
		ID3D12GraphicsCommandList2*	m_CommandList		= nullptr;
		bool						m_IsBeingRecorded	= false;
	};

	CommandListEntry			m_CommandListEntries[NUM_BUFFERED_FRAMES];

public:
	DX12CommandQueue(DX12Device* inDevice, D3D12_COMMAND_LIST_TYPE inType);
	virtual ~DX12CommandQueue();

	// Get an available command list from the command queue.
	ID3D12GraphicsCommandList2* GetCommandList(DX12Device* inDevice);

	// Execute a command list.
	// Returns the fence value to wait for for this command list.
	ui64 ExecuteCommandList(ID3D12GraphicsCommandList2* inCommandList);

	ui64 Signal();
	bool IsFenceComplete(ui64 fenceValue) const;
	void WaitForFenceValue(ui64 fenceValue) const;
	void Flush();

	ID3D12CommandQueue* GetD3D12CommandQueue() const;

protected:
	ID3D12CommandAllocator*		CreateCommandAllocator(DX12Device* inDevice) const;
	ID3D12GraphicsCommandList2* CreateCommandList(DX12Device* inDevice, ID3D12CommandAllocator* inCommandAllocator) const;
};