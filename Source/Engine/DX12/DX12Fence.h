#pragma once

#include <d3d12.h>
#include <chrono>

class DX12Fence
{
protected:
	ID3D12Fence*	m_Fence;
	uint64			m_FenceValue = 0;
	HANDLE			m_FenceEvent;

public:
	DX12Fence(ID3D12Device* inDevice);
	virtual ~DX12Fence();

	uint64 Signal(ID3D12CommandQueue* inCommandQueue);
	void WaitForFenceValue(uint64 inFenceValue, std::chrono::milliseconds inDuration = (std::chrono::milliseconds::max)()) const;
	bool IsFenceComplete(uint64 inFenceValue) const;
};