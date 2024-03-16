#pragma once

#include "Common.h"

class Renderer
{
public:
	Renderer();
	~Renderer() = default;

	HRESULT InitDevice(_In_ HWND hWnd);

	void Render();

private:
	static const UINT NUM_FRAME_BUFFERS = 2;

	// Pipeline objects
	ComPtr<ID3D12Device> m_pDevice;
	ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	ComPtr<IDXGISwapChain3> m_pSwapChain;
	ComPtr<ID3D12DescriptorHeap> m_pRtvHeap;
	ComPtr<ID3D12Resource> m_apRenderTargets[NUM_FRAME_BUFFERS];
	ComPtr<ID3D12CommandAllocator> m_pCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList;
	UINT m_uRtvDescriptorSize;

	// Synchronization objects
	UINT m_uFrameIndex;
	HANDLE m_hFenceEvent;
	ComPtr<ID3D12Fence> m_pFence;
	UINT64 m_uFenceValue;
};