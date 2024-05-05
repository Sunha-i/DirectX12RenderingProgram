#pragma once

#include "Common.h"

#include "Texture/WICTextureLoader.h"
#include "Texture/ResourceUploadBatch.h"

class Texture
{
public:
	Texture(_In_ const std::filesystem::path& filePath);
	~Texture() = default;

	HRESULT Initialize(
		_In_ ComPtr<ID3D12Device>& pDevice,
		_In_ ComPtr<ID3D12CommandQueue>& pCommandQueue,
		_In_ ComPtr<ID3D12DescriptorHeap>& pSrvHeap
	);

private:
	std::filesystem::path m_filePath;

	ComPtr<ID3D12Resource> m_pTextureResource;
};