#include "Engine.h"
#include "DX12Fence.h"

HANDLE CreateEventHandle()
{
	HANDLE fence_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	Assert(fence_event, "Failed to create fence event.");

	return fence_event;
}

DX12Fence::DX12Fence(ID3D12Device* inDevice)
{
	ThrowIfFailed(inDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
	m_FenceEvent = CreateEventHandle();
}

DX12Fence::~DX12Fence()
{
	m_Fence->Release();
	::CloseHandle(m_FenceEvent);
}

uint64 DX12Fence::Signal(ID3D12CommandQueue* inCommandQueue)
{
	uint64 fence_value_for_signal = ++m_FenceValue;
	ThrowIfFailed(inCommandQueue->Signal(m_Fence, fence_value_for_signal));

	return fence_value_for_signal;
}

void DX12Fence::WaitForFenceValue(uint64 inFenceValue, std::chrono::milliseconds inDuration/* = std::chrono::milliseconds::max()*/) const
{
	if (m_Fence->GetCompletedValue() < inFenceValue)
	{
		ThrowIfFailed(m_Fence->SetEventOnCompletion(inFenceValue, m_FenceEvent));
		::WaitForSingleObject(m_FenceEvent, static_cast<DWORD>(inDuration.count()));
	}
}

bool DX12Fence::IsFenceComplete(uint64 inFenceValue) const
{
	return m_Fence->GetCompletedValue() >= inFenceValue;
}