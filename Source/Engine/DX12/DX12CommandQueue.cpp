#include "Engine.h"
#include "DX12/DX12CommandQueue.h"

#include "DX12/DX12Device.h"
#include "DX12/DX12SwapChain.h"

DX12CommandQueue::DX12CommandQueue(DX12Device& inDevice, D3D12_COMMAND_LIST_TYPE inType) :
	m_CommandListType(inType),
	m_Fence(inDevice.m_Device)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type		= inType;
	desc.Priority	= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags		= D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask	= 0;

	ThrowIfFailed(inDevice.m_Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_CommandQueue)));
}

DX12CommandQueue::~DX12CommandQueue()
{
	Flush();

	for (auto& command_list_entry : m_CommandListEntries)
	{
		if (command_list_entry.m_CommandList)
		{
			command_list_entry.m_CommandList->Release();
			command_list_entry.m_CommandAllocator->Release();
		}
	}

	m_CommandQueue->Release();
}

uint64_t DX12CommandQueue::Signal()
{
	return m_Fence.Signal(m_CommandQueue);
}

bool DX12CommandQueue::IsFenceComplete(uint64 inFenceValue) const
{
	return m_Fence.IsFenceComplete(inFenceValue);
}

void DX12CommandQueue::WaitForFenceValue(uint64 inFenceValue) const
{
	m_Fence.WaitForFenceValue(inFenceValue);
}

void DX12CommandQueue::Flush()
{
	WaitForFenceValue(Signal());
}

ID3D12CommandAllocator* DX12CommandQueue::CreateCommandAllocator(DX12Device& inDevice) const
{
	ID3D12CommandAllocator* command_allocator;
	ThrowIfFailed(inDevice.m_Device->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&command_allocator)));

	return command_allocator;
}

ID3D12GraphicsCommandList2* DX12CommandQueue::CreateCommandList(DX12Device& inDevice, ID3D12CommandAllocator* inCommandAllocator) const
{
	ID3D12GraphicsCommandList2* command_list;
	ThrowIfFailed(inDevice.m_Device->CreateCommandList(0, m_CommandListType, inCommandAllocator, nullptr, IID_PPV_ARGS(&command_list)));

	return command_list;
}

ID3D12GraphicsCommandList2* DX12CommandQueue::GetCommandList(DX12Device& inDevice)
{
	m_CurrentIndex = inDevice.m_SwapChain->GetCurrentBackBufferIndex();

	// No need for fences, a commandlist from 3 frames ago should already have been processed by the GPU
	auto& entry = m_CommandListEntries[m_CurrentIndex];

	// Test to see if it's worth destroying and recreating command list allocators to free memory.
	// It doesn't seem to change anything, so let's disable it for now
#define ALLOCATE_NEW_COMMANDLISTS 0

	if (!entry.m_IsBeingRecorded)
	{
		entry.m_IsBeingRecorded = true;

		if (entry.m_CommandList)
		{
#if !ALLOCATE_NEW_COMMANDLISTS
			entry.m_CommandAllocator->Reset();
			entry.m_CommandList->Reset(entry.m_CommandAllocator, nullptr);
#else
			entry.m_CommandList->Release();
			entry.commandAllocator->Release();
#endif
		}
#if !ALLOCATE_NEW_COMMANDLISTS
		else
#endif
		{
			entry.m_CommandAllocator	= CreateCommandAllocator(inDevice);
			entry.m_CommandList			= CreateCommandList(inDevice, entry.m_CommandAllocator);
		}
	}

	return entry.m_CommandList;
}

// Execute a command list.
// Returns the fence value to wait for for this command list.
uint64 DX12CommandQueue::ExecuteCommandList(ID3D12GraphicsCommandList2* inCommandList)
{
	inCommandList->Close();

	ID3D12CommandList* const ppCommandLists[] =
	{
		inCommandList
	};

	m_CommandQueue->ExecuteCommandLists(1, ppCommandLists);
	uint64_t fence_value = Signal();

	auto& entry = m_CommandListEntries[m_CurrentIndex];
	// Make sure we are executing a commandlist from the correct frame
	Assert(entry.m_CommandList == inCommandList);

	entry.m_IsBeingRecorded = false;

	return fence_value;
}

ID3D12CommandQueue* DX12CommandQueue::GetD3D12CommandQueue() const
{
	return m_CommandQueue;
}