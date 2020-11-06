#pragma once

// Make sure DX12Includes is included before DirectXTex
#include "DX12/DX12Includes.h"
#include <DirectXTex/DirectXTex.h>

class DX12Texture;

class TextureLoader
{
protected:
	DirectX::ScratchImage m_ScratchImage;

public:
	void			LoadFromFile(const std::string& inFile);
	DX12Texture*	CreateTexture(ID3D12GraphicsCommandList2* inCommandList);

	// Static members
public:
	static void		Init();
};