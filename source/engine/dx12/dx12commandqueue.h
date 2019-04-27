#pragma once

#include "datatypes.h"
#include "dx12fence.h"

#include <d3d12.h>

#include <queue>

class DX12Device;

class DX12CommandQueue
{
public:
	DX12CommandQueue(DX12Device* device, D3D12_COMMAND_LIST_TYPE type);
	virtual ~DX12CommandQueue();

	// Get an available command list from the command queue.
	ID3D12GraphicsCommandList2* GetCommandList(DX12Device* device);

	// Execute a command list.
	// Returns the fence value to wait for for this command list.
	ui64 ExecuteCommandList(ID3D12GraphicsCommandList2* commandList);

	ui64 Signal();
	bool IsFenceComplete(ui64 fenceValue);
	void WaitForFenceValue(ui64 fenceValue);
	void Flush();

	ID3D12CommandQueue* GetD3D12CommandQueue() const;
protected:

	ID3D12CommandAllocator* CreateCommandAllocator(DX12Device* device);
	ID3D12GraphicsCommandList2* CreateCommandList(DX12Device* device, ID3D12CommandAllocator* allocator);

private:
	// Keep track of command allocators that are "in-flight"
	struct CommandAllocatorEntry
	{
		ui64 fenceValue;
		ID3D12CommandAllocator* commandAllocator;
	};

	using CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;
	using CommandListQueue = std::queue<ID3D12GraphicsCommandList2*>;

	D3D12_COMMAND_LIST_TYPE		m_CommandListType;
	ID3D12CommandQueue*			m_d3d12CommandQueue;

	DX12Fence					m_Fence;
	ui64						m_FenceValue;

	CommandAllocatorQueue		m_CommandAllocatorQueue;
	CommandListQueue			m_CommandListQueue;
};