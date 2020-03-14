#pragma once

#include <d3d12.h>
#include <vector>

class DX12Device;

class DX12DescriptorHeap
{
protected:
	ID3D12DescriptorHeap*			m_DescriptorHeap	= nullptr;
	uint32							m_IncrementSize;
	uint32							m_NumDescriptors;
	uint32							m_CurrentOffset		= 0;

	D3D12_CPU_DESCRIPTOR_HANDLE		m_CPUHandle			= { 0 };
	D3D12_GPU_DESCRIPTOR_HANDLE		m_GPUHandle			= { 0 };

public:
	DX12DescriptorHeap(
		DX12Device& inDevice,
		D3D12_DESCRIPTOR_HEAP_TYPE inHeapType, uint32 inNumDescriptors,
		D3D12_DESCRIPTOR_HEAP_FLAGS inFlags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	virtual ~DX12DescriptorHeap();

	virtual uint32 GetFreeIndex();

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32 inIndex) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32 inIndex) const;
	ID3D12DescriptorHeap*		GetD3DDescriptorHeap() const;
};

// Same as a descriptor heap except of allocating the next entry in the heap, we find the first entry in the heap
// Useful when allocating/deallocating resources in and out of the heap
class DX12FreeListDescriptorHeap : public DX12DescriptorHeap
{
protected:
	std::vector<bool> m_FreeEntries;

public:
	DX12FreeListDescriptorHeap(
		DX12Device& inDevice,
		D3D12_DESCRIPTOR_HEAP_TYPE inHeapType, uint32 inNumDescriptors,
		D3D12_DESCRIPTOR_HEAP_FLAGS inFlags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	virtual ~DX12FreeListDescriptorHeap();

	virtual uint32 GetFreeIndex();
	void ReleaseIndex(uint32 inIndex);
};