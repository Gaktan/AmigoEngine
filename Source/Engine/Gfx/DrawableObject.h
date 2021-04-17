#pragma once

#include "DX12/DX12Includes.h"

class Mesh;
class ShaderObject;

class DrawableObject final
{
public:
	DrawableObject(Mesh* inMesh, const ShaderObject* inShaderObjet);
	~DrawableObject();

	void SetupBindings(ID3D12GraphicsCommandList2* inCommandList);
	void Render(ID3D12GraphicsCommandList2* inCommandList);

private:
	// TODO: The DrawableObject currently has ownership of the Mesh
	Mesh*				m_Mesh		= nullptr;
	const ShaderObject*	m_Shader	= nullptr;
};