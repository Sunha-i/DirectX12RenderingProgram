#include "Renderable.h"

Renderable::Renderable(_In_ const XMFLOAT4& outputColor)
	: m_vOutputColor(outputColor)
	, m_mWorld(XMMatrixIdentity())
	, m_pVertexBuffer()
	, m_pIndexBuffer()
	, m_vertexBufferView()
	, m_indexBufferView()
{
}

HRESULT Renderable::initialize(_In_ ComPtr<ID3D12Device>& pDevice)
{
	HRESULT hr = S_OK;

	hr = createVertexBuffer(pDevice);
	if (FAILED(hr))
		return hr;
	hr = createIndexBuffer(pDevice);
	if (FAILED(hr))
		return hr;

	return hr;
}

HRESULT Renderable::createVertexBuffer(_In_ ComPtr<ID3D12Device>& pDevice)
{
	HRESULT hr = S_OK;

	const Vertex* cubeVertices = getVertices();
	const UINT uVertexBufferSize = sizeof(Vertex) * GetNumVertices();

	// Create committed resource for vertex buffer
	const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
	const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uVertexBufferSize);
	hr = pDevice->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_pVertexBuffer)
	);
	if (FAILED(hr))
	{
		return hr;
	}
	// Copy the cube data to the vertex buffer
	UINT8* pVertexDataBegin;
	CD3DX12_RANGE readRange(0, 0);
	hr = m_pVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
	if (FAILED(hr))
	{
		return hr;
	}
	memcpy(pVertexDataBegin, cubeVertices, uVertexBufferSize);
	m_pVertexBuffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view
	m_vertexBufferView =
	{
		.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress(),
		.SizeInBytes = uVertexBufferSize,
		.StrideInBytes = sizeof(Vertex),
	};

	return hr;
}

HRESULT Renderable::createIndexBuffer(_In_ ComPtr<ID3D12Device>& pDevice)
{
	HRESULT hr = S_OK;

	const WORD* cubeIndices = getIndices();
	const UINT uIndexBufferSize = sizeof(WORD) * GetNumIndices();

	// Create committed resource for index buffer
	const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
	const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uIndexBufferSize);
	hr = pDevice->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_pIndexBuffer)
	);
	if (FAILED(hr))
	{
		return hr;
	}
	// Copy array of index data to upload buffer
	UINT8* pIndexDataBegin;
	CD3DX12_RANGE readRange(0, 0);
	hr = m_pIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
	if (FAILED(hr))
	{
		return hr;
	}
	memcpy(pIndexDataBegin, cubeIndices, uIndexBufferSize);
	m_pIndexBuffer->Unmap(0, nullptr);

	// Initialize the index buffer view
	m_indexBufferView =
	{
		.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress(),
		.SizeInBytes = uIndexBufferSize,
		.Format = DXGI_FORMAT_R16_UINT,
	};

	return hr;
}

D3D12_VERTEX_BUFFER_VIEW* Renderable::GetVertexBufferView()
{
	return &m_vertexBufferView;
}

D3D12_INDEX_BUFFER_VIEW* Renderable::GetIndexBufferView()
{
	return &m_indexBufferView;
}

const XMMATRIX& Renderable::GetWorldMatrix() const
{
	return m_mWorld;
}

const XMFLOAT4& Renderable::GetOutputColor() const
{
	return m_vOutputColor;
}

BOOL Renderable::HasTexture() const
{
	if (m_aMaterials.size() > 0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

const Material& Renderable::GetMaterial(UINT uIndex)
{
	assert(uIndex < GetNumMaterials());

	return m_aMaterials[uIndex];
}

const Renderable::BasicMeshEntry& Renderable::GetMesh(UINT uIndex)
{
	assert(uIndex < GetNumMeshes());

	return m_aMeshes[uIndex];
}

void Renderable::RotateX(_In_ FLOAT angle)
{
	m_mWorld *= XMMatrixRotationX(angle);
}

void Renderable::RotateY(_In_ FLOAT angle)
{
	m_mWorld *= XMMatrixRotationY(angle);
}

void Renderable::RotateZ(_In_ FLOAT angle)
{
	m_mWorld *= XMMatrixRotationZ(angle);
}

void Renderable::RotateRollPitchYaw(_In_ FLOAT roll, _In_ FLOAT pitch, _In_ FLOAT yaw)
{
	m_mWorld *= XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
}

void Renderable::Scale(_In_ FLOAT scaleX, _In_ FLOAT scaleY, _In_ FLOAT scaleZ)
{
	m_mWorld *= XMMatrixScaling(scaleX, scaleY, scaleZ);
}

void Renderable::Translate(_In_ const XMVECTOR& offset)
{
	m_mWorld *= XMMatrixTranslationFromVector(offset);
}

UINT Renderable::GetNumMeshes() const
{
	return static_cast<UINT>(m_aMeshes.size());
}

UINT Renderable::GetNumMaterials() const
{
	return static_cast<UINT>(m_aMaterials.size());
}
