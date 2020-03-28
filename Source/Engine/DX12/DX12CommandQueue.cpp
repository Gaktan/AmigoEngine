#include "Engine.h"
#include "DX12/DX12CommandQueue.h"

#include "DX12/DX12Device.h"
#include "DX12/DX12SwapChain.h"

DX12CommandQueue::DX12CommandQueue(DX12Device& inDevice, D3D12_COMMAND_LIST_TYPE inType) :
	m_CommandListType(inType),
	m_Fence(inDevice)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type		= inType;
	desc.Priority	= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags		= D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask	= 0;

	ThrowIfFailed(inDevice.GetD3DDevice()->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_D3DCommandQueue)));

	for (auto& entry : m_CommandListEntries)
	{
		entry.m_D3DCommandAllocator	= CreateCommandAllocator(inDevice);
		entry.m_D3DCommandList		= CreateCommandList(inDevice, entry.m_D3DCommandAllocator);
	}
}

DX12CommandQueue::~DX12CommandQueue()
{
	Flush();

	for (auto& command_list_entry : m_CommandListEntries)
	{
		if (command_list_entry.m_D3DCommandList)
		{
			command_list_entry.m_D3DCommandList->Release();
			command_list_entry.m_D3DCommandAllocator->Release();
		}
	}

	m_D3DCommandQueue->Release();
}

uint64_t DX12CommandQueue::Signal()
{
	return m_Fence.Signal(m_D3DCommandQueue);
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
	ThrowIfFailed(inDevice.GetD3DDevice()->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&command_allocator)));

	return command_allocator;
}

ID3D12GraphicsCommandList2* DX12CommandQueue::CreateCommandList(DX12Device& inDevice, ID3D12CommandAllocator* inCommandAllocator) const
{
	ID3D12GraphicsCommandList2* command_list;
	ThrowIfFailed(inDevice.GetD3DDevice()->CreateCommandList(0, m_CommandListType, inCommandAllocator, nullptr, IID_PPV_ARGS(&command_list)));

	command_list->Close();

	return command_list;
}

ID3D12GraphicsCommandList2* DX12CommandQueue::GetCommandList(DX12Device& inDevice)
{
	// TODO: Store frame index elsewhere
	m_CurrentIndex = inDevice.GetSwapChain()->GetCurrentBackBufferIndex();

	// No need for fences, a commandlist from 3 frames ago should already have been processed by the GPU
	auto& entry = m_CommandListEntries[m_CurrentIndex];

	if (!entry.m_IsBeingRecorded)
	{
		entry.m_IsBeingRecorded = true;

		entry.m_D3DCommandAllocator->Reset();
		entry.m_D3DCommandList->Reset(entry.m_D3DCommandAllocator, nullptr);
	}

	return entry.m_D3DCommandList;
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

	m_D3DCommandQueue->ExecuteCommandLists(1, ppCommandLists);
	uint64_t fence_value = Signal();

	auto& entry = m_CommandListEntries[m_CurrentIndex];
	// Make sure we are executing a commandlist from the correct frame
	Assert(entry.m_D3DCommandList == inCommandList);

	entry.m_IsBeingRecorded = false;

	return fence_value;
}

ID3D12CommandQueue* DX12CommandQueue::GetD3D12CommandQueue() const
{
	return m_D3DCommandQueue;
}