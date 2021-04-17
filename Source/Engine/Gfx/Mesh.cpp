#include "Engine.h"
#include "Mesh.h"

void Mesh::Init(
	ID3D12GraphicsCommandList2* inCommandList,
	D3D_PRIMITIVE_TOPOLOGY inPrimitiveTopology,
	void* inVertexBuffer, int32 inVertexBufferSize, int32 inStride,
	void* inIndexBuffer/* = nullptr*/, int32 inIndexBufferSize/* = 0*/)
{
	m_VertexBuffer = new DX12VertexBuffer;
	m_VertexBuffer->InitAsVertexBuffer(inCommandList, inVertexBufferSize, inVertexBuffer, inStride);

	if (inIndexBuffer != nullptr)
	{
		m_IndexBuffer = new DX12IndexBuffer;
		m_IndexBuffer->InitAsIndexBuffer(inCommandList, inIndexBufferSize, inIndexBuffer);
		m_NumIndices = inIndexBufferSize / sizeof(uint16);
	}

	m_PrimitiveTopology = inPrimitiveTopology;
}

void Mesh::Release()
{
	m_VertexBuffer->Release();
	delete m_VertexBuffer;

	if (m_IndexBuffer != nullptr)
	{
		m_IndexBuffer->Release();
		delete m_IndexBuffer;
	}
}

void Mesh::SetResourceName(const std::string& inName)
{
	// TODO: Properly handle string and wstrings...
	const std::wstring wide_str(inName.begin(), inName.end());

	m_VertexBuffer->GetResource()->SetName((wide_str + L"_VertexBuffer").c_str());

	if (m_IndexBuffer != nullptr)
		m_IndexBuffer->GetResource()->SetName((wide_str + L"_IndexBuffer").c_str());
}

void Mesh::Set(ID3D12GraphicsCommandList2* inCommandList) const
{
	inCommandList->IASetPrimitiveTopology(m_PrimitiveTopology);

	m_VertexBuffer->SetVertexBuffer(inCommandList, 0);

	if (m_IndexBuffer != nullptr)
		m_IndexBuffer->SetIndexBuffer(inCommandList);
}
