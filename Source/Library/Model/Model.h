#pragma once

#include "Common.h"

#include "Renderer/Renderable.h"

struct aiScene;
struct aiMesh;
struct aiMaterial;

class Model : public Renderable
{
public:
	Model(_In_ const std::filesystem::path& filePath);
	virtual ~Model() = default;

	virtual HRESULT Initialize(
		_In_ ComPtr<ID3D12Device>& pDevice,
		_In_ ComPtr<ID3D12CommandQueue>& pCommandQueue,
		_In_ ComPtr<ID3D12DescriptorHeap>& pSrvHeap
	) override;
	virtual void Update(_In_ FLOAT deltaTime) override;

	UINT GetNumVertices() const override;
	UINT GetNumIndices() const override;

protected:
	std::filesystem::path m_filePath;
	std::vector<Vertex> m_aVertices;
	std::vector<WORD> m_aIndices;

	const Vertex* getVertices() const override;
	const WORD* getIndices() const override;

	HRESULT initFromScene(
		_In_ ComPtr<ID3D12Device>& pDevice,
		_In_ ComPtr<ID3D12CommandQueue>& pCommandQueue,
		_In_ ComPtr<ID3D12DescriptorHeap>& pSrvHeap,
		_In_ const aiScene* pScene,
		_In_ const std::filesystem::path& filePath
	);

	void countVerticesAndIndices(_Inout_ UINT& uOutNumVertices, _Inout_ UINT& uOutNumIndices, _In_ const aiScene* pScene);
	void reserveSpace(_In_ UINT uNumVertices, _In_ UINT uNumIndices);
	void initAllMeshes(_In_ const aiScene* pScene);
	void initSingleMesh(_In_ const aiMesh* pMesh);

	HRESULT initMaterials(
		_In_ ComPtr<ID3D12Device>& pDevice,
		_In_ ComPtr<ID3D12CommandQueue>& pCommandQueue,
		_In_ ComPtr<ID3D12DescriptorHeap>& pSrvHeap,
		_In_ const aiScene* pScene,
		_In_ const std::filesystem::path& filePath
	);

	HRESULT loadTextures(
		_In_ ComPtr<ID3D12Device>& pDevice,
		_In_ ComPtr<ID3D12CommandQueue>& pCommandQueue,
		_In_ ComPtr<ID3D12DescriptorHeap>& pSrvHeap,
		_In_ const std::filesystem::path& parentDirectory,
		_In_ const aiMaterial* pMaterial,
		_In_ UINT uIndex
	);

	HRESULT loadDiffuseTexture(
		_In_ ComPtr<ID3D12Device>& pDevice,
		_In_ ComPtr<ID3D12CommandQueue>& pCommandQueue,
		_In_ ComPtr<ID3D12DescriptorHeap>& pSrvHeap,
		_In_ const std::filesystem::path& parentDirectory,
		_In_ const aiMaterial* pMaterial, _In_ UINT uIndex
	);

	HRESULT loadSpecularTexture(
		_In_ ComPtr<ID3D12Device>& pDevice,
		_In_ ComPtr<ID3D12CommandQueue>& pCommandQueue,
		_In_ ComPtr<ID3D12DescriptorHeap>& pSrvHeap,
		_In_ const std::filesystem::path& parentDirectory,
		_In_ const aiMaterial* pMaterial, _In_ UINT uIndex
	);

	void loadColors(_In_ const aiMaterial* pMaterial, _In_ UINT uIndex);
};