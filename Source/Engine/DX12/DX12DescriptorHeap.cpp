#include "Engine.h"
#include "DX12/DX12DescriptorHeap.h"

#include "DX12/DX12Device.h"

DX12DescriptorHeap::DX12DescriptorHeap(
	D3D12_DESCRIPTOR_HEAP_TYPE inHeapType, uint32 inNumDescriptors,
	D3D12_DESCRIPTOR_HEAP_FLAGS inFlags/* = D3D12_DESCRIPTOR_HEAP_FLAG_NONE*/)
{
	m_NumDescriptors = inNumDescriptors;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors	= inNumDescriptors;
	desc.Type			= inHeapType;
	desc.NodeMask		= 0;
	desc.Flags			= inFlags;
	ThrowIfFailed(g_RenderingDevice.GetD3DDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_D3DDescriptorHeap)));

	m_IncrementSize	= g_RenderingDevice.GetD3DDevice()->GetDescriptorHandleIncrementSize(inHeapType);
	m_CPUHandle		= m_D3DDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_GPUHandle		= m_D3DDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
}

DX12DescriptorHeap::~DX12DescriptorHeap()
{
	m_D3DDescriptorHeap->Release();
}

uint32 DX12DescriptorHeap::Allocate()
{
	std::lock_guard<std::mutex> lock(m_FreeIndexMutex);

	Assert(m_CurrentOffset < m_NumDescriptors);
	return m_CurrentOffset++;
}

void DX12DescriptorHeap::Release(uint32 inIndex)
{
	// Nothing to do...
	(void) inIndex;
}

void DX12DescriptorHeap::Release(D3D12_CPU_DESCRIPTOR_HANDLE inHandle)
{
	// Nothing to do...
	(void) inHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::GetCPUHandle(uint32 inIndex) const
{
	Assert(inIndex < m_NumDescriptors);

	// From CD3DX12_CPU_DESCRIPTOR_HANDLE::Offset
	D3D12_CPU_DESCRIPTOR_HANDLE handle;
	handle.ptr = m_CPUHandle.ptr + int64(inIndex) * int64(m_IncrementSize);

	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::GetGPUHandle(uint32 inIndex) const
{
	Assert(inIndex < m_NumDescriptors);

	// From CD3DX12_GPU_DESCRIPTOR_HANDLE::Offset
	D3D12_GPU_DESCRIPTOR_HANDLE handle;
	handle.ptr = m_GPUHandle.ptr + int64(inIndex) * int64(m_IncrementSize);

	return handle;
}

ID3D12DescriptorHeap* DX12DescriptorHeap::GetD3DDescriptorHeap() const
{
	return m_D3DDescriptorHeap;
}

DX12FreeListDescriptorHeap::DX12FreeListDescriptorHeap(
	D3D12_DESCRIPTOR_HEAP_TYPE inHeapType, uint32 inNumDescriptors,
	D3D12_DESCRIPTOR_HEAP_FLAGS inFlags/* = D3D12_DESCRIPTOR_HEAP_FLAG_NONE*/) :
	DX12DescriptorHeap(inHeapType, inNumDescriptors, inFlags)
{
	m_FreeIndices.reserve(inNumDescriptors);
	for (uint32 i = 0; i < inNumDescriptors; ++i)
		m_FreeIndices.push_back(i);
}

DX12FreeListDescriptorHeap::~DX12FreeListDescriptorHeap()
{
}

uint32 DX12FreeListDescriptorHeap::Allocate()
{
	uint32 free_index = 0;

	// Find a free spot
	{
		std::lock_guard<std::mutex> lock(m_FreeIndexMutex);

		// Should have enough entries available
		Assert(!m_FreeIndices.empty());

		// Retrieve the back entry and pop the back (required LOCK)
		free_index = m_FreeIndices.back();
		m_FreeIndices.pop_back();
	}

	return free_index;
}

void DX12FreeListDescriptorHeap::Release(uint32 inIndex)
{
	Assert(inIndex < m_NumDescriptors);

	// TODO: Might want to check that we don't release the same index twice?

	// Add to free list again
	std::lock_guard<std::mutex> lock(m_FreeIndexMutex);
	m_FreeIndices.push_back(inIndex);
}

void DX12FreeListDescriptorHeap::Release(D3D12_CPU_DESCRIPTOR_HANDLE inHandle)
{
	if (inHandle.ptr == 0)
		return;

	// Convert the CPU handle back to an index (Inverse of GetCPUHandle)
	uint64 index = (inHandle.ptr - m_CPUHandle.ptr) / uint64(m_IncrementSize);
	Assert(index < m_NumDescriptors);

	Release((uint32)index);
}