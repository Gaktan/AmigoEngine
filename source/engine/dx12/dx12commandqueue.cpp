#include "engine_precomp.h"
#include "dx12commandqueue.h"

#include "dx12device.h"

#include <cassert>
#include <exception>

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

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

	while (!m_CommandAllocatorQueue.empty())
	{
		auto* commandAllocator = m_CommandAllocatorQueue.front().commandAllocator;
		m_CommandAllocatorQueue.pop();
		commandAllocator->Release();
	}

	while (!m_CommandListQueue.empty())
	{
		auto* commandList = m_CommandListQueue.front();
		m_CommandListQueue.pop();
		commandList->Release();
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
	ID3D12CommandAllocator* commandAllocator;
	ID3D12GraphicsCommandList2* commandList;

	if (!m_CommandAllocatorQueue.empty() && IsFenceComplete(m_CommandAllocatorQueue.front().fenceValue))
	{
		commandAllocator = m_CommandAllocatorQueue.front().commandAllocator;
		m_CommandAllocatorQueue.pop();

		ThrowIfFailed(commandAllocator->Reset());
	}
	else
	{
		commandAllocator = CreateCommandAllocator(device);
	}

	if (!m_CommandListQueue.empty())
	{
		commandList = m_CommandListQueue.front();
		m_CommandListQueue.pop();

		ThrowIfFailed(commandList->Reset(commandAllocator, nullptr));
	}
	else
	{
		commandList = CreateCommandList(device, commandAllocator);
	}

	// Associate the command allocator with the command list so that it can be
	// retrieved when the command list is executed.
	ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator));

	return commandList;
}

// Execute a command list.
// Returns the fence value to wait for for this command list.
ui64 DX12CommandQueue::ExecuteCommandList(ID3D12GraphicsCommandList2* commandList)
{
	commandList->Close();

	ID3D12CommandAllocator* commandAllocator;
	UINT dataSize = sizeof(commandAllocator);
	ThrowIfFailed(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));

	ID3D12CommandList* const ppCommandLists[] =
	{
		commandList
	};

	m_CommandQueue->ExecuteCommandLists(1, ppCommandLists);
	uint64_t fenceValue = Signal();

	m_CommandAllocatorQueue.emplace(CommandAllocatorEntry{ fenceValue, commandAllocator });
	m_CommandListQueue.push(commandList);

	commandAllocator->Release();

	return fenceValue;
}

ID3D12CommandQueue* DX12CommandQueue::GetD3D12CommandQueue() const
{
	return m_CommandQueue;
}