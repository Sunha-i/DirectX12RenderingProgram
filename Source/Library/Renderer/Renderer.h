#pragma once

#include "Common.h"

#include "Renderer/DataTypes.h"

class Renderer
{
public:
	Renderer();
	~Renderer() = default;

	HRESULT InitDevice(_In_ HWND hWnd);

	void Update();
	void Render();
	void WaitForPreviousFrame();

private:
	static const UINT NUM_FRAME_BUFFERS = 2;

	// Pipeline objects
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	ComPtr<ID3D12Device> m_pDevice;
	ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	ComPtr<IDXGISwapChain3> m_pSwapChain;
	ComPtr<ID3D12DescriptorHeap> m_pRtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_pCbvHeap;
	ComPtr<ID3D12Resource> m_apRenderTargets[NUM_FRAME_BUFFERS];
	ComPtr<ID3D12CommandAllocator> m_pCommandAllocator;
	ComPtr<ID3D12RootSignature> m_pRootSignature;
	ComPtr<ID3D12PipelineState> m_pPipelineState;
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList;
	UINT m_uRtvDescriptorSize;

	// App resources
	ComPtr<ID3D12Resource> m_pVertexBuffer;
	ComPtr<ID3D12Resource> m_pConstantBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	SceneConstantBuffer m_constantBufferData;
	UINT8* m_upCbvDataBegin;

	// Synchronization objects
	UINT m_uFrameIndex;
	HANDLE m_hFenceEvent;
	ComPtr<ID3D12Fence> m_pFence;
	UINT64 m_uFenceValue;
};