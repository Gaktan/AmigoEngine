#pragma once

#include "DX12/DX12Device.h"

class Mesh;
class ShaderObject;

class DrawableObject
{
protected:
	// TODO: The DrawableObject currently has ownership of the Mesh
	const Mesh*			m_Mesh		= nullptr;
	const ShaderObject*	m_Shader	= nullptr;

public:
	DrawableObject(const Mesh* inMesh, const ShaderObject* inShaderObjet);
	~DrawableObject();

	void SetupBindings(ID3D12GraphicsCommandList2* inCommandList);
	void Render(ID3D12GraphicsCommandList2* inCommandList);
};