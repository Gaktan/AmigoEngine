#pragma once

#include "DX12/DX12Includes.h"
#include <vector>
#include <mutex>

class DX12DescriptorHeap
{
	friend class DX12Device;
	friend class DX12CommandQueue;

protected:
	DX12DescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE inHeapType, uint32 inNumDescriptors,
		D3D12_DESCRIPTOR_HEAP_FLAGS inFlags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	virtual ~DX12DescriptorHeap();

	void Reset();

public:
	virtual uint32	Allocate();
	virtual void	Release(uint32 inIndex);
	virtual void	Release(D3D12_CPU_DESCRIPTOR_HANDLE inHandle);

	D3D12_CPU_DESCRIPTOR_HANDLE		GetCPUHandle(uint32 inIndex) const;
	D3D12_GPU_DESCRIPTOR_HANDLE		GetGPUHandle(uint32 inIndex) const;
	inline ID3D12DescriptorHeap&	GetD3DDescriptorHeap() const			{ return *m_D3DDescriptorHeap; }

private:
	ID3D12DescriptorHeap*			m_D3DDescriptorHeap	= nullptr;

protected:
	uint32							m_IncrementSize;
	uint32							m_NumDescriptors;
	uint32							m_CurrentOffset		= 0;

	D3D12_CPU_DESCRIPTOR_HANDLE		m_CPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE		m_GPUHandle;

	std::mutex						m_FreeIndexMutex;
};

// Same as a descriptor heap except of allocating the next entry in the heap, we find the first entry in the heap
// Useful when allocating/deallocating resources in and out of the heap
class DX12FreeListDescriptorHeap final : public DX12DescriptorHeap
{
	friend class DX12Device;

private:
	DX12FreeListDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE inHeapType, uint32 inNumDescriptors,
		D3D12_DESCRIPTOR_HEAP_FLAGS inFlags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	~DX12FreeListDescriptorHeap() override = default;

public:
	virtual uint32	Allocate() override;
	virtual void	Release(uint32 inIndex) override;
	virtual void	Release(D3D12_CPU_DESCRIPTOR_HANDLE inHandle) override;

private:
	std::vector<uint32> m_FreeIndices;
};