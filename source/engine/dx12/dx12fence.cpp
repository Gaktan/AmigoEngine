#include "engine_precomp.h"
#include "dx12fence.h"

#include<cassert>
#include <exception>

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

HANDLE CreateEventHandle()
{
	HANDLE fenceEvent;

	fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent && "Failed to create fence event.");

	return fenceEvent;
}

DX12Fence::DX12Fence(ID3D12Device* device)
{
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
	m_FenceEvent = CreateEventHandle();
}

DX12Fence::~DX12Fence()
{
	m_Fence->Release();
}

ui64 DX12Fence::Signal(ID3D12CommandQueue* commandQueue)
{
	ui64 fenceValueForSignal = ++m_FenceValue;
	ThrowIfFailed(commandQueue->Signal(m_Fence, fenceValueForSignal));

	return fenceValueForSignal;
}

void DX12Fence::WaitForFenceValue(ui64 fenceValue, std::chrono::milliseconds duration/* = std::chrono::milliseconds::max()*/)
{
	if (m_Fence->GetCompletedValue() < fenceValue)
	{
		ThrowIfFailed(m_Fence->SetEventOnCompletion(fenceValue, m_FenceEvent));
		::WaitForSingleObject(m_FenceEvent, static_cast<DWORD>(duration.count()));
	}
}