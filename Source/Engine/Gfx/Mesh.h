#pragma once

#include "DX12/DX12Resource.h"

class Mesh final
{
public:
	void Init(
		ID3D12GraphicsCommandList2& inCommandList,
		D3D_PRIMITIVE_TOPOLOGY inPrimitiveTopology,
		void* inVertexBuffer, int32 inVertexBufferSize, int32 inStride,
		void* inIndexBuffer = nullptr, int32 inIndexBufferSize = 0);

	void	Release();

	void	SetResourceName(const std::string& inName);

	void			Set(ID3D12GraphicsCommandList2& inCommandList) const;
	inline uint32	GetNumIndices() const			{ return m_NumIndices; }

private:
	DX12VertexBuffer*			m_VertexBuffer;
	DX12IndexBuffer*			m_IndexBuffer;
	uint32						m_NumIndices		= 0;
	D3D_PRIMITIVE_TOPOLOGY		m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
};