#pragma once

#include "Common.h"

#include "Renderer/Renderable.h"
#include "Light/PointLight.h"
#include "Texture/DDSTextureLoader.h"
#include "Texture/ResourceUploadBatch.h"

class Renderer
{
public:
	Renderer();
	~Renderer() = default;

	HRESULT InitDevice(_In_ HWND hWnd);
	HRESULT AddRenderable(_In_ PCWSTR pszRenderableName, _In_ const std::shared_ptr<Renderable>& renderable);
	HRESULT AddPointLight(_In_ size_t index, _In_ const std::shared_ptr<PointLight>& pPointLight);

	void Update(_In_ FLOAT deltaTime);
	void Render();
	void WaitForGpu();
	void MoveToNextFrame();

private:
	static const UINT NUM_FRAME_BUFFERS = 2;
	static const UINT NUM_DRAW_CALLS = 3;

	// Pipeline objects
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	ComPtr<ID3D12Device> m_pDevice;
	ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	ComPtr<IDXGISwapChain3> m_pSwapChain;
	ComPtr<ID3D12DescriptorHeap> m_pRtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_pDsvHeap;
	ComPtr<ID3D12DescriptorHeap> m_pSrvHeap;
	ComPtr<ID3D12Resource> m_pDepthStencil;
	ComPtr<ID3D12Resource> m_apRenderTargets[NUM_FRAME_BUFFERS];
	ComPtr<ID3D12CommandAllocator> m_apCommandAllocators[NUM_FRAME_BUFFERS];
	ComPtr<ID3D12RootSignature> m_pRootSignature;
	ComPtr<ID3D12PipelineState> m_pLambertPipelineState;
	ComPtr<ID3D12PipelineState> m_pSolidPipelineState;
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList;

	// App resources
	ComPtr<ID3D12Resource> m_pVertexBuffer;
	ComPtr<ID3D12Resource> m_pIndexBuffer;
	ComPtr<ID3D12Resource> m_pTextureResource;
	ComPtr<ID3D12Resource> m_pPerFrameConstants;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	D3D12_GPU_VIRTUAL_ADDRESS m_constantDataGpuAddr;
	ConstantBuffer* m_mappedConstantData;
	UINT m_uRtvDescriptorSize;

	// Synchronization objects
	UINT m_uFrameIndex;
	HANDLE m_hFenceEvent;
	ComPtr<ID3D12Fence> m_pFence;
	UINT64 m_auFenceValues[NUM_FRAME_BUFFERS];

	// Computed values 'll be loaded into CB
	XMMATRIX m_worldMatrix;
	XMMATRIX m_viewMatrix;
	XMMATRIX m_projectionMatrix;
	XMFLOAT4 m_vOutputColor;

	std::unordered_map<PCWSTR, std::shared_ptr<Renderable>> m_umRenderables;
	std::shared_ptr<PointLight> m_apPointLights[NUM_LIGHTS];
};