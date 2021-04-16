#include "Engine.h"
#include "DrawableObject.h"

#include "DX12/DX12Includes.h"

#include "Gfx/Mesh.h"
#include "Gfx/RenderPass.h"
#include "Gfx/ShaderObject.h"

DrawableObject::DrawableObject(Mesh* inMesh, const ShaderObject* inShaderObjet) :
	m_Mesh(inMesh),
	m_Shader(inShaderObjet)
{
	Assert(m_Mesh != nullptr);
	Assert(m_Shader != nullptr);
}

DrawableObject::~DrawableObject()
{
	m_Mesh->Release();
	delete m_Mesh;
}

void DrawableObject::SetupBindings(ID3D12GraphicsCommandList2* inCommandList)
{
	m_Shader->Set(inCommandList);
	m_Mesh->Set(inCommandList);
}

void DrawableObject::Render(ID3D12GraphicsCommandList2* inCommandList)
{
	inCommandList->DrawIndexedInstanced(m_Mesh->GetNumIndices(), 1, 0, 0, 0);
}

