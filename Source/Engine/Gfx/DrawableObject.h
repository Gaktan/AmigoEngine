#pragma once

#include "DX12/DX12Device.h"

#include "Gfx/RenderPass.h"

class Mesh;

class DrawableObject
{
protected:
	// TODO: The DrawableObject currently has ownership of the Mesh
	Mesh*					m_Mesh			= nullptr;
	ID3D12PipelineState*	m_PipelineState	= nullptr;
	ID3D12RootSignature*	m_RootSignature = nullptr;
	RenderPass				m_RenderPass;

public:
	virtual ~DrawableObject();

	static DrawableObject* CreateDrawableObject(DX12Device& inDevice, Mesh* inMesh, RenderPass inRenderPass);

	void SetupBindings(ID3D12GraphicsCommandList2* inCommandList);
	void Render(ID3D12GraphicsCommandList2* inCommandList);

private:
	void CreatePSO(DX12Device& inDevice);
	void CreateRootSignature(DX12Device& inDevice);
};