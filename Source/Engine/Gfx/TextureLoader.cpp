#include "Engine.h"
#include "TextureLoader.h"

#include "DX12/DX12Texture.h"

#include "Utils/FileReader.h"

void TextureLoader::LoadFromFile(const std::string& inFile)
{
	FileReader file_reader;
	bool success = file_reader.ReadFile(inFile);
	Assert(success);

	HRESULT result = DirectX::LoadFromWICMemory(
		file_reader.GetContentAsBinary(), file_reader.GetContentSize(),
		DirectX::WIC_FLAGS_NONE, nullptr, m_ScratchImage);

	ThrowIfFailed(result);

	DirectX::TexMetadata metadata = m_ScratchImage.GetMetadata();
	Assert(Math::IsPowerOfTwo(static_cast<int>(metadata.width)));
	Assert(Math::IsPowerOfTwo(static_cast<int>(metadata.height)));
}

DX12Texture* TextureLoader::CreateTexture(DX12Device& inDevice, ID3D12GraphicsCommandList2* inCommandList)
{
	DirectX::TexMetadata metadata		= m_ScratchImage.GetMetadata();
	DX12DescriptorHeap* descriptor_heap	= inDevice.GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	DX12Texture* texture = new DX12Texture();
	texture->InitAsTexture(inDevice, inCommandList, descriptor_heap,
						   static_cast<uint32>(metadata.width), static_cast<uint32>((metadata.height)),
						   metadata.format, m_ScratchImage.GetPixels());

	return texture;
}

void TextureLoader::Init()
{
	// DirectXTex requires this
	HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
	ThrowIfFailed(hr);
}
