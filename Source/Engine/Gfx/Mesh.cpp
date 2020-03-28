#include "Engine.h"
#include "Mesh.h"

void Mesh::Init(
	DX12Device& inDevice, ID3D12GraphicsCommandList2* inCommandList,
	D3D_PRIMITIVE_TOPOLOGY inPrimitiveTopology,
	void* inVertexBuffer, int32 inVertexBufferSize, int32 inStride,
	void* inIndexBuffer, int32 inIndexBufferSize)
{
	m_VertexBuffer.InitAsVertexBuffer(inDevice, inCommandList, inVertexBufferSize, inVertexBuffer, inStride);
	m_IndexBuffer.InitAsIndexBuffer(inDevice, inCommandList, inIndexBufferSize, inIndexBuffer);

	m_NumIndices		= inIndexBufferSize / sizeof(uint16);
	m_PrimitiveTopology = inPrimitiveTopology;
}

void Mesh::SetResourceName(const std::string & inName)
{
	// TODO: Properly handle string and wstrings...
	const std::wstring wide_str(inName.begin(), inName.end());
	m_VertexBuffer.GetResource()->SetName((wide_str + L"_VertexBuffer").c_str());
	m_IndexBuffer.GetResource()->SetName((wide_str + L"_IndexBuffer").c_str());
}

void Mesh::Set(ID3D12GraphicsCommandList2* inCommandList)
{
	inCommandList->IASetPrimitiveTopology(m_PrimitiveTopology);
	m_VertexBuffer.SetVertexBuffer(inCommandList, 0);
	m_IndexBuffer.SetIndexBuffer(inCommandList);
}

uint32 Mesh::GetNumIndices() const
{
	return m_NumIndices;
}
