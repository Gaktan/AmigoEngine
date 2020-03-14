#include "Engine.h"
#include "DX12/DX12DescriptorHeap.h"

#include "DX12/DX12Device.h"

DX12DescriptorHeap::DX12DescriptorHeap(
	DX12Device& inDevice,
	D3D12_DESCRIPTOR_HEAP_TYPE inHeapType, uint32 inNumDescriptors,
	D3D12_DESCRIPTOR_HEAP_FLAGS inFlags/* = D3D12_DESCRIPTOR_HEAP_FLAG_NONE*/)
{
	m_NumDescriptors = inNumDescriptors;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors	= inNumDescriptors;
	desc.Type			= inHeapType;
	desc.NodeMask		= 0;
	desc.Flags			= inFlags;
	ThrowIfFailed(inDevice.m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_DescriptorHeap)));

	m_IncrementSize	= inDevice.m_Device->GetDescriptorHandleIncrementSize(inHeapType);
	m_CPUHandle		= m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_GPUHandle		= m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
}

DX12DescriptorHeap::~DX12DescriptorHeap()
{
	m_DescriptorHeap->Release();
}

uint32 DX12DescriptorHeap::GetFreeIndex()
{
	Assert(m_CurrentOffset < m_NumDescriptors);
	return m_CurrentOffset++;
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

	// From CD3DX12_CPU_DESCRIPTOR_HANDLE::Offset
	D3D12_GPU_DESCRIPTOR_HANDLE handle;
	handle.ptr = m_GPUHandle.ptr + int64(inIndex) * int64(m_IncrementSize);

	return handle;
}

ID3D12DescriptorHeap* DX12DescriptorHeap::GetD3DDescriptorHeap() const
{
	return m_DescriptorHeap;
}
