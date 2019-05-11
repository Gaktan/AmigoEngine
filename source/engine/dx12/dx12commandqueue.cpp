#include "engine_precomp.h"
#include "dx12commandqueue.h"

#include "dx12/dx12device.h"
#include "dx12/dx12swapchain.h"

DX12CommandQueue::DX12CommandQueue(DX12Device* device, D3D12_COMMAND_LIST_TYPE type)
	: m_CommandListType(type)
	, m_Fence(device->m_Device)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(device->m_Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_CommandQueue)));
}

DX12CommandQueue::~DX12CommandQueue()
{
	Flush();
	
	for (auto& commandListEntry : m_CommandListEntries)
	{
		if (commandListEntry.commandList)
		{
			commandListEntry.commandList->Release();
			commandListEntry.commandAllocator->Release();
		}
	}

	m_CommandQueue->Release();
}

uint64_t DX12CommandQueue::Signal()
{
	return m_Fence.Signal(m_CommandQueue);
}

bool DX12CommandQueue::IsFenceComplete(ui64 fenceValue) const
{
	return m_Fence.IsFenceComplete(fenceValue);
}

void DX12CommandQueue::WaitForFenceValue(ui64 fenceValue) const
{
	m_Fence.WaitForFenceValue(fenceValue);
}

void DX12CommandQueue::Flush()
{
	WaitForFenceValue(Signal());
}

ID3D12CommandAllocator* DX12CommandQueue::CreateCommandAllocator(DX12Device* device) const
{
	ID3D12CommandAllocator* commandAllocator;
	ThrowIfFailed(device->m_Device->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&commandAllocator)));

	return commandAllocator;
}

ID3D12GraphicsCommandList2* DX12CommandQueue::CreateCommandList(DX12Device* device, ID3D12CommandAllocator* allocator) const
{
	ID3D12GraphicsCommandList2* commandList;
	ThrowIfFailed(device->m_Device->CreateCommandList(0, m_CommandListType, allocator, nullptr, IID_PPV_ARGS(&commandList)));

	return commandList;
}

ID3D12GraphicsCommandList2* DX12CommandQueue::GetCommandList(DX12Device* device)
{
	m_CurrentIndex = device->m_SwapChain->GetCurrentBackBufferIndex();

	// No need for fences, a commandlist from 3 frames ago should already have been processed by the GPU
	auto& entry = m_CommandListEntries[m_CurrentIndex];

	// Test to see if it's worth destroying and recreating command list allocators to free memory.
	// It doesn't seem to change anything, so let's disable it for now
#define ALLOCATE_NEW_COMMANDLISTS 0

	if (!entry.isBeingRecorded)
	{
		entry.isBeingRecorded = true;

		if (entry.commandList)
		{
#if !ALLOCATE_NEW_COMMANDLISTS
			entry.commandAllocator->Reset();
			entry.commandList->Reset(entry.commandAllocator, nullptr);
#else
			entry.commandList->Release();
			entry.commandAllocator->Release();
#endif
		}
#if !ALLOCATE_NEW_COMMANDLISTS
		else
#endif
		{
			entry.commandAllocator = CreateCommandAllocator(device);
			entry.commandList = CreateCommandList(device, entry.commandAllocator);
		}
	}

	return entry.commandList;
}

// Execute a command list.
// Returns the fence value to wait for for this command list.
ui64 DX12CommandQueue::ExecuteCommandList(ID3D12GraphicsCommandList2* commandList)
{
	commandList->Close();

	ID3D12CommandList* const ppCommandLists[] =
	{
		commandList
	};

	m_CommandQueue->ExecuteCommandLists(1, ppCommandLists);
	uint64_t fenceValue = Signal();

	auto& entry = m_CommandListEntries[m_CurrentIndex];
	// Make sure we are executing a commandlist from the correct frame
	Assert(entry.commandList == commandList);

	entry.isBeingRecorded = false;

	return fenceValue;
}

ID3D12CommandQueue* DX12CommandQueue::GetD3D12CommandQueue() const
{
	return m_CommandQueue;
}