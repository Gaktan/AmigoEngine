#include "Engine.h"
#include "DX12/DX12Resource.h"

#include "DX12/DX12Device.h"

void DX12Resource::InitAsResource(
	ID3D12GraphicsCommandList2* inCommandList,
	size_t inBufferSize/* = 0*/, const void* inBufferData/* = nullptr*/,
	D3D12_RESOURCE_FLAGS inFlags/* = D3D12_RESOURCE_FLAG_NONE*/)
{
	D3D12_HEAP_PROPERTIES	heap_properties	= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC		resource_desc	= CD3DX12_RESOURCE_DESC::Buffer(inBufferSize, inFlags);

	// Create a committed resource for the GPU resource in a default heap.
	ThrowIfFailed(g_RenderingDevice.GetD3DDevice()->CreateCommittedResource(
		&heap_properties,
		D3D12_HEAP_FLAG_NONE,
		&resource_desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_Resource)));

	UpdateBufferResource(inCommandList, inBufferSize, inBufferData);

	SetResourceName(m_Resource, "DX12Resource::InitAsResource");
}

void DX12Resource::ReleaseResources()
{
	if (m_Resource != nullptr)
		m_Resource->Release();

	if (m_IntermediateResource != nullptr)
		m_IntermediateResource->Release();
}

DX12Resource::~DX12Resource()
{
	ReleaseResources();
}

void DX12Resource::UpdateBufferResource(
	ID3D12GraphicsCommandList2* inCommandList,
	size_t inBufferSize/* = 0*/, const void* inBufferData/* = nullptr*/)
{
	// Create an committed resource for the upload.
	if (inBufferData)
	{
		if (m_IntermediateResource == nullptr)
		{
			D3D12_HEAP_PROPERTIES	heap_properties	= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			D3D12_RESOURCE_DESC		resource_desc	= CD3DX12_RESOURCE_DESC::Buffer(inBufferSize);

			// Create upload buffer on the CPU
			ThrowIfFailed(g_RenderingDevice.GetD3DDevice()->CreateCommittedResource(
				&heap_properties,
				D3D12_HEAP_FLAG_NONE,
				&resource_desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_IntermediateResource)));

			SetResourceName(m_IntermediateResource, "DX12Resource::UpdateBufferResource");
		}

		D3D12_SUBRESOURCE_DATA subresource_data = {};
		subresource_data.pData		= inBufferData;
		subresource_data.RowPitch	= inBufferSize;
		subresource_data.SlicePitch	= subresource_data.RowPitch;

		UpdateSubresources(inCommandList,
						   m_Resource, m_IntermediateResource,
						   0, 0, 1, &subresource_data);
	}
}

ID3D12Resource* DX12Resource::GetResource() const
{
	return m_Resource;
}

void DX12Resource::SetResourceName(ID3D12Resource* inResource, const std::string& inName)
{
	static uint32 resource_number = 0;
	std::string narrow_string = inName + "_" + std::to_string(resource_number++);
	std::wstring wide_string = std::wstring(narrow_string.begin(), narrow_string.end());
	inResource->SetName(wide_string.c_str());
}

void DX12VertexBuffer::InitAsVertexBuffer(
	ID3D12GraphicsCommandList2* inCommandList,
	size_t inBufferSize, const void* inBufferData, uint32 inStride,
	D3D12_RESOURCE_FLAGS inFlags/* = D3D12_RESOURCE_FLAG_NONE*/)
{
	DX12Resource::InitAsResource(inCommandList, inBufferSize, inBufferData, inFlags);

	m_VertexBufferView.BufferLocation	= m_Resource->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes		= (uint32) inBufferSize;
	m_VertexBufferView.StrideInBytes	= inStride;

	const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_Resource,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	inCommandList->ResourceBarrier(1, &barrier);

	SetResourceName(m_Resource, "DX12VertexBuffer::InitAsVertexBuffer");
}

void DX12VertexBuffer::SetVertexBuffer(ID3D12GraphicsCommandList2* inCommandList, uint32 inStartSlot) const
{
	inCommandList->IASetVertexBuffers(inStartSlot, 1, &m_VertexBufferView);
}

void DX12IndexBuffer::InitAsIndexBuffer(
	ID3D12GraphicsCommandList2* inCommandList,
	size_t inBufferSize, const void* inBufferData,
	D3D12_RESOURCE_FLAGS inFlags/* = D3D12_RESOURCE_FLAG_NONE*/)
{
	DX12Resource::InitAsResource(inCommandList, inBufferSize, inBufferData, inFlags);

	m_IndexBufferView.BufferLocation	= m_Resource->GetGPUVirtualAddress();
	m_IndexBufferView.Format			= DXGI_FORMAT_R16_UINT;
	m_IndexBufferView.SizeInBytes		= (uint32) inBufferSize;

	const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_Resource,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

	inCommandList->ResourceBarrier(1, &barrier);

	SetResourceName(m_Resource, "DX12IndexBuffer::InitAsIndexBuffer");
}

void DX12IndexBuffer::SetIndexBuffer(ID3D12GraphicsCommandList2* inCommandList) const
{
	inCommandList->IASetIndexBuffer(&m_IndexBufferView);
}

void DX12ConstantBuffer::InitAsConstantBuffer(size_t inBufferSize)
{
	D3D12_HEAP_PROPERTIES	heap_properties	= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC		resource_desc	= CD3DX12_RESOURCE_DESC::Buffer(inBufferSize);

	// These will remain in upload heap because we use them only once per frame
	ThrowIfFailed(g_RenderingDevice.GetD3DDevice()->CreateCommittedResource(
		&heap_properties,
		D3D12_HEAP_FLAG_NONE,
		&resource_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_Resource)));

	SetResourceName(m_Resource, "DX12ConstantBuffer::InitAsConstantBuffer");
}

void DX12ConstantBuffer::UpdateBufferResource(
	ID3D12GraphicsCommandList2* inCommandList,
	size_t inBufferSize/* = 0*/, const void* inBufferData/* = nullptr*/)
{
	// TODO: Ignore unused arguments
	(void) inCommandList;

	// Perform Map/Unmap with the new data
	if (inBufferData)
	{
		void* p;
		m_Resource->Map(0, nullptr, &p);
		::memcpy(p, inBufferData, inBufferSize);
		m_Resource->Unmap(0, nullptr);
	}
}

void DX12ConstantBuffer::SetConstantBuffer(ID3D12GraphicsCommandList2* inCommandList, uint32 inRootParameterIndex) const
{
	inCommandList->SetGraphicsRootConstantBufferView(inRootParameterIndex, m_Resource->GetGPUVirtualAddress());
}