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

	virtual HRESULT Initialize();
	virtual void Update(_In_ FLOAT deltaTime) override;

	UINT GetNumVertices() const override;
	UINT GetNumIndices() const override;

protected:
	std::filesystem::path m_filePath;
	std::vector<Vertex> m_aVertices;
	std::vector<WORD> m_aIndices;

	const Vertex* getVertices() const override;
	const WORD* getIndices() const override;
};