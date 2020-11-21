#include "Engine.h"
#include "DX12/DX12CommandQueue.h"

#include "DX12/DX12Device.h"
#include "DX12/DX12SwapChain.h"

DX12CommandQueue::DX12CommandQueue(D3D12_COMMAND_LIST_TYPE inType) :
	m_CommandListType(inType),
	m_Fence()
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type		= inType;
	desc.Priority	= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags		= D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask	= 0;

	ThrowIfFailed(g_RenderingDevice.GetD3DDevice()->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_D3DCommandQueue)));

	for (auto& entry : m_CommandListEntries)
	{
		entry.m_D3DCommandAllocator	= CreateCommandAllocator();
		entry.m_D3DCommandList		= CreateCommandList(entry.m_D3DCommandAllocator);
		entry.m_DescriptorHeap		= new DX12DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2048, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	}
}

DX12CommandQueue::~DX12CommandQueue()
{
	Flush();

	for (auto& entry : m_CommandListEntries)
	{
		if (entry.m_D3DCommandList)
		{
			entry.m_D3DCommandList->Release();
			entry.m_D3DCommandAllocator->Release();
			delete entry.m_DescriptorHeap;
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

ID3D12CommandAllocator* DX12CommandQueue::CreateCommandAllocator() const
{
	ID3D12CommandAllocator* command_allocator;
	ThrowIfFailed(g_RenderingDevice.GetD3DDevice()->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&command_allocator)));

	return command_allocator;
}

ID3D12GraphicsCommandList2* DX12CommandQueue::CreateCommandList(ID3D12CommandAllocator* inCommandAllocator) const
{
	ID3D12GraphicsCommandList2* command_list;
	ThrowIfFailed(g_RenderingDevice.GetD3DDevice()->CreateCommandList(0, m_CommandListType, inCommandAllocator, nullptr, IID_PPV_ARGS(&command_list)));

	command_list->Close();

	return command_list;
}

ID3D12GraphicsCommandList2* DX12CommandQueue::GetCommandList()
{
	m_CurrentIndex = g_RenderingDevice.GetFrameID() % NUM_BUFFERED_FRAMES;

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

DX12DescriptorHeap& DX12CommandQueue::GetDescriptorHeap()
{
	return *m_CommandListEntries[m_CurrentIndex].m_DescriptorHeap;
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

	// Reset descriptor heap for shader bindings
	entry.m_DescriptorHeap->Reset();

	return fence_value;
}