#include "BaseCube.h"

BaseCube::BaseCube(_In_ const XMFLOAT4& outputColor)
	: Renderable(outputColor)
{
}

HRESULT BaseCube::Initialize(
	_In_ ComPtr<ID3D12Device>& pDevice,
	_In_ ComPtr<ID3D12CommandQueue>& pCommandQueue,
	_In_ ComPtr<ID3D12DescriptorHeap>& pSrvHeap
){
	BasicMeshEntry basicMeshEntry;
	basicMeshEntry.uNumIndices = NUM_INDICES;
	basicMeshEntry.uMaterialIndex = 0u;

	m_aMeshes.push_back(basicMeshEntry);
	m_aMaterials.push_back(Material());

	return initialize(pDevice);
}

UINT BaseCube::GetNumVertices() const
{
	return NUM_VERTICES;
}

UINT BaseCube::GetNumIndices() const
{
	return NUM_INDICES;
}

const Vertex* BaseCube::getVertices() const
{
	return VERTICES;
}

const WORD* BaseCube::getIndices() const
{
	return INDICES;
}