#pragma once

#include "DX12/DX12Includes.h"
#include <chrono>

class DX12Fence final
{
	friend class DX12CommandQueue;

public:
	DX12Fence();
	~DX12Fence();

private:
	inline ID3D12Fence&		GetD3DFence() const				{ return *m_D3DFence; }

public:
	uint64					Increment();
	void					WaitForFenceValue(uint64 inFenceValue, std::chrono::milliseconds inDuration = (std::chrono::milliseconds::max)()) const;
	bool					IsFenceComplete(uint64 inFenceValue) const;

private:
	ID3D12Fence*	m_D3DFence;
	uint64			m_FenceValue	= 0;
	HANDLE			m_FenceEvent;
};