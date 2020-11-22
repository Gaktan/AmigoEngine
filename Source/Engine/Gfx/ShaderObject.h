#pragma once

#include "Gfx/RenderPass.h"

// A ShaderObject actually regroups multiple shaders. (e.g Pixel+Vertex shader)
// In this case, shaders a grouped into a single PSO
class ShaderObject
{
protected:
	ID3D12PipelineState*	m_PipelineState	= nullptr;
	ID3D12RootSignature*	m_RootSignature = nullptr;
	RenderPass				m_RenderPass;

	std::string				m_Name;

public:
	ShaderObject(RenderPass inRenderPass, const D3D12_SHADER_BYTECODE inVSBytecode, const D3D12_SHADER_BYTECODE inPSBytecode);
	~ShaderObject();

	const std::string&	GetName() const			{ return m_Name; }
	RenderPass			GetRenderPass() const	{ return m_RenderPass; }

	void Set(ID3D12GraphicsCommandList2* inCommandList) const;

private:
	void CreatePSO(const D3D12_SHADER_BYTECODE inVSBytecode, const D3D12_SHADER_BYTECODE inPSBytecode);
	void CreateRootSignature();
};