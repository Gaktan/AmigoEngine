#include "Engine.h"
#include "DX12/DX12Fence.h"

#include "DX12/DX12Device.h"

HANDLE CreateEventHandle()
{
	HANDLE fence_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	Assert(fence_event, "Failed to create fence event.");

	return fence_event;
}

DX12Fence::DX12Fence()
{
	ThrowIfFailed(g_RenderingDevice.GetD3DDevice().CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_D3DFence)));
	m_FenceEvent = CreateEventHandle();
}

DX12Fence::~DX12Fence()
{
	m_D3DFence->Release();
	::CloseHandle(m_FenceEvent);
}

uint64 DX12Fence::Increment()
{
	return ++m_FenceValue;
}

void DX12Fence::WaitForFenceValue(uint64 inFenceValue, std::chrono::milliseconds inDuration/* = std::chrono::milliseconds::max()*/) const
{
	if (m_D3DFence->GetCompletedValue() < inFenceValue)
	{
		ThrowIfFailed(m_D3DFence->SetEventOnCompletion(inFenceValue, m_FenceEvent));
		::WaitForSingleObject(m_FenceEvent, static_cast<DWORD>(inDuration.count()));
	}
}

bool DX12Fence::IsFenceComplete(uint64 inFenceValue) const
{
	return m_D3DFence->GetCompletedValue() >= inFenceValue;
}