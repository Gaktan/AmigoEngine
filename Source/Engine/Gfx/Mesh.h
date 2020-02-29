#pragma once

#include "DX12/DX12Resource.h"

class Mesh
{
protected:
	DX12VertexBuffer			m_VertexBuffer;
	DX12IndexBuffer				m_IndexBuffer;
	D3D_PRIMITIVE_TOPOLOGY		m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

public:
	virtual ~Mesh()
	{
	}

	void Init(
		ID3D12Device* inDevice, ID3D12GraphicsCommandList2* inCommandList,
		D3D_PRIMITIVE_TOPOLOGY inPrimitiveTopology,
		void* inVertexBuffer, int32 inVertexBufferSize, int32 inStride,
		void* inIndexBuffer, int32 inIndexBufferSize);

	void Set(ID3D12GraphicsCommandList2* inCommandList);
};