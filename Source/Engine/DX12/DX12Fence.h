#pragma once

#include <d3d12.h>
#include <chrono>

class DX12Fence
{
protected:
	ID3D12Fence*	m_Fence;
	ui64			m_FenceValue = 0;
	HANDLE			m_FenceEvent;

public:
	DX12Fence(ID3D12Device* inDevice);
	virtual ~DX12Fence();

	ui64 Signal(ID3D12CommandQueue* inCommandQueue);
	void WaitForFenceValue(ui64 inFenceValue, std::chrono::milliseconds inDuration = (std::chrono::milliseconds::max)()) const;
	bool IsFenceComplete(ui64 inFenceValue) const;
};