#pragma once

// Make sure DX12Includes is included before DirectXTex
#include "DX12/DX12Includes.h"
#include <DirectXTex/DirectXTex.h>

class DX12Texture;

class TextureLoader final
{
public:
	void			LoadFromFile(const std::string& inFile);
	DX12Texture*	CreateTexture(ID3D12GraphicsCommandList2* inCommandList);

public:
	static void		Init();

private:
	DirectX::ScratchImage m_ScratchImage;
};