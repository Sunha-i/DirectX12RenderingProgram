#pragma once

#include "Common.h"

#include "Renderer/DataTypes.h"
#include "Material/Material.h"

class Renderable
{
public:
	static constexpr const UINT INVALID_MATERIAL = (0xFFFFFFFF);

protected:
	struct BasicMeshEntry {
		BasicMeshEntry()
			: uNumIndices(0u)
			, uBaseVertex(0u)
			, uBaseIndex(0u)
			, uMaterialIndex(INVALID_MATERIAL)
		{}
		UINT uNumIndices;
		UINT uBaseVertex;
		UINT uBaseIndex;
		UINT uMaterialIndex;
	};

public:
	Renderable(_In_ const XMFLOAT4& outputColor);
	Renderable(const Renderable& other) = delete;
	Renderable(Renderable&& other) = delete;
	Renderable() = default;
	virtual ~Renderable() = default;

	virtual HRESULT Initialize(
		_In_ ComPtr<ID3D12Device>& pDevice,
		_In_ ComPtr<ID3D12CommandQueue>& pCommandQueue,
		_In_ ComPtr<ID3D12DescriptorHeap>& pSrvHeap
	) = 0;
	virtual void Update(_In_ FLOAT deltaTime) = 0;

	D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView();
	D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView();

	const XMMATRIX& GetWorldMatrix() const;
	const XMFLOAT4& GetOutputColor() const;

	void RotateX(_In_ FLOAT angle);
	void RotateY(_In_ FLOAT angle);
	void RotateZ(_In_ FLOAT angle);
	void RotateRollPitchYaw(_In_ FLOAT roll, _In_ FLOAT pitch, _In_ FLOAT yaw);
	void Scale(_In_ FLOAT scaleX, _In_ FLOAT scaleY, _In_ FLOAT scaleZ);
	void Translate(_In_ const XMVECTOR& offset);

	virtual UINT GetNumVertices() const = 0;
	virtual UINT GetNumIndices() const = 0;

protected:
	virtual const Vertex* getVertices() const = 0;
	virtual const WORD* getIndices() const = 0;

	HRESULT initialize(_In_ ComPtr<ID3D12Device>& pDevice);
	HRESULT createVertexBuffer(_In_ ComPtr<ID3D12Device>& pDevice);
	HRESULT createIndexBuffer(_In_ ComPtr<ID3D12Device>& pDevice);

	ComPtr<ID3D12Resource> m_pVertexBuffer;
	ComPtr<ID3D12Resource> m_pIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	std::vector<BasicMeshEntry> m_aMeshes;
	std::vector<Material> m_aMaterials;

	XMFLOAT4 m_vOutputColor;
	XMMATRIX m_mWorld;
};