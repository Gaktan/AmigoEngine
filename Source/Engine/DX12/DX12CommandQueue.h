#pragma once

#include "DX12/DX12Device.h"
#include "DX12/DX12Fence.h"
// For NUM_BUFFERED_FRAMES
#include "DX12/DX12SwapChain.h"

#include <queue>

class DX12CommandQueue
{
protected:
	D3D12_COMMAND_LIST_TYPE		m_CommandListType;
	ID3D12CommandQueue*			m_D3DCommandQueue;

	DX12Fence					m_Fence;
	uint32						m_CurrentIndex;

	struct CommandListEntry
	{
		ID3D12CommandAllocator*		m_D3DCommandAllocator	= nullptr;
		ID3D12GraphicsCommandList2*	m_D3DCommandList		= nullptr;
		bool						m_IsBeingRecorded		= false;
	};

	CommandListEntry			m_CommandListEntries[NUM_BUFFERED_FRAMES];

public:
	DX12CommandQueue(DX12Device& inDevice, D3D12_COMMAND_LIST_TYPE inType);
	virtual ~DX12CommandQueue();

	// Get an available command list from the command queue.
	ID3D12GraphicsCommandList2* GetCommandList(DX12Device& inDevice);

	// Execute a command list.
	// Returns the fence value to wait for for this command list.
	uint64 ExecuteCommandList(ID3D12GraphicsCommandList2* inCommandList);

	uint64 Signal();
	bool IsFenceComplete(uint64 fenceValue) const;
	void WaitForFenceValue(uint64 fenceValue) const;
	void Flush();

	ID3D12CommandQueue* GetD3D12CommandQueue() const;

protected:
	ID3D12CommandAllocator*		CreateCommandAllocator(DX12Device& inDevice) const;
	ID3D12GraphicsCommandList2* CreateCommandList(DX12Device& inDevice, ID3D12CommandAllocator* inCommandAllocator) const;
};