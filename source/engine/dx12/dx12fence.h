#pragma once

#include "datatypes.h"

#include <d3d12.h>
#include <chrono>

class DX12Fence
{
protected:
	ID3D12Fence*	m_Fence;
	ui64			m_FenceValue = 0;
	HANDLE			m_FenceEvent;

public:
	DX12Fence(ID3D12Device* device);
	virtual ~DX12Fence();

	ui64 Signal(ID3D12CommandQueue* commandQueue);
	void WaitForFenceValue(ui64 fenceValue, std::chrono::milliseconds duration = (std::chrono::milliseconds::max)());
};