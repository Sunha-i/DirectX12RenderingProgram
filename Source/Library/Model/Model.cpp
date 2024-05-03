#include "Model/Model.h"

#include "assimp/Importer.hpp"		// C++ importer interface
#include "assimp/scene.h"			// output data structure
#include "assimp/postprocess.h"		// post processing flags

Model::Model(_In_ const std::filesystem::path& filePath)
	: Renderable(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f))
	, m_filePath(filePath)
{
}

HRESULT Model::Initialize()
{
	HRESULT hr = S_OK;

	Assimp::Importer importer;

	const aiScene* pScene = importer.ReadFile(
		m_filePath.string().c_str(),
		aiProcess_Triangulate | aiProcess_ConvertToLeftHanded
	);

	// For Debugging
	static CHAR szDebugMessage[256];
	sprintf_s(szDebugMessage, "Number of meshes found in file: %d\nNumber of verticies in first mesh: %d\n", 
				pScene->mNumMeshes, pScene->mMeshes[0]->mNumVertices);

	OutputDebugStringA(szDebugMessage);

	return hr;
}

void Model::Update(_In_ FLOAT deltaTime)
{
}

UINT Model::GetNumVertices() const
{
	return static_cast<UINT>(m_aVertices.size());
}

UINT Model::GetNumIndices() const
{
	return static_cast<UINT>(m_aIndices.size());
}

const Vertex* Model::getVertices() const
{
	return m_aVertices.data();
}

const WORD* Model::getIndices() const
{
	return m_aIndices.data();
}