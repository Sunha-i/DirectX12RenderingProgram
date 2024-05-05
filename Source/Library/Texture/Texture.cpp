#include "Texture/Texture.h"

Texture::Texture(_In_ const std::filesystem::path& filePath)
	: m_filePath(filePath)
	, m_pTextureResource()
{
}

HRESULT Texture::Initialize(
	_In_ ComPtr<ID3D12Device>& pDevice,
	_In_ ComPtr<ID3D12CommandQueue>& pCommandQueue,
	_In_ ComPtr<ID3D12DescriptorHeap>& pSrvHeap
){
	HRESULT hr = S_OK;

	// start recording upload commands
	ResourceUploadBatch resourceUpload(pDevice.Get());
	resourceUpload.Begin();

	// loads a texture
	m_pTextureResource.Reset();
	hr = CreateWICTextureFromFile(pDevice.Get(), resourceUpload, m_filePath.c_str(), m_pTextureResource.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		OutputDebugString(L"Can't load texture from \"");
		OutputDebugString(m_filePath.c_str());
		OutputDebugString(L"\n");

		return hr;
	}
	// upload resources to the GPU and wait for the upload thread to terminate
	auto uploadFin = resourceUpload.End(pCommandQueue.Get());
	uploadFin.wait();

	// describe and create a SRV for the texture
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc =
	{
		.Format = m_pTextureResource->GetDesc().Format,
		.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Texture2D = { .MipLevels = 1 }
	};
	pDevice->CreateShaderResourceView(m_pTextureResource.Get(), &srvDesc, pSrvHeap->GetCPUDescriptorHandleForHeapStart());

	return hr;
}
