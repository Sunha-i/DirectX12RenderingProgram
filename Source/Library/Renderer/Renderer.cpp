#include "Renderer/Renderer.h"

Renderer::Renderer()
	: m_pDevice()
	, m_pCommandQueue()
	, m_pSwapChain()
	, m_pRtvHeap()
	, m_pCommandList()
	, m_uRtvDescriptorSize(0)
	, m_uFrameIndex(0)
	, m_hFenceEvent()
	, m_uFenceValue(0)
{
}

HRESULT Renderer::InitDevice(_In_ HWND hWnd)
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(hWnd, &rc);
	UINT uWidth = static_cast<UINT>(rc.right - rc.left);
	UINT uHeight = static_cast<UINT>(rc.bottom - rc.top);

	DWORD dwDebugFactoryFlags = 0;

#if defined(_DEBUG)
	// Enable the debug layer
	{
		ComPtr<ID3D12Debug> pDebugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(pDebugController.GetAddressOf()))))
		{
			pDebugController->EnableDebugLayer();

			dwDebugFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif
	// Create DXGI factory
	ComPtr<IDXGIFactory6> pDxgiFactory;
	hr = CreateDXGIFactory2(dwDebugFactoryFlags, IID_PPV_ARGS(pDxgiFactory.GetAddressOf()));
	if (FAILED(hr))
	{
		return hr;
	}

	// Scan DXGI adapters
	ComPtr<IDXGIAdapter1> pAdapter1;
	ComPtr<IDXGIAdapter4> pAdapter;

	DXGI_GPU_PREFERENCE activeGpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
	for (UINT uAdapterIdx = 0;
		DXGI_ERROR_NOT_FOUND != pDxgiFactory->EnumAdapterByGpuPreference
		(uAdapterIdx, activeGpuPreference, IID_PPV_ARGS(pAdapter1.ReleaseAndGetAddressOf()));
		++uAdapterIdx)
	{
		DXGI_ADAPTER_DESC1 desc;
		hr = pAdapter1->GetDesc1(&desc);
		if (FAILED(hr))
		{
			continue;
		}

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter aka WARP+VGA driver
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet
		if (SUCCEEDED(D3D12CreateDevice(pAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
		{
			hr = pAdapter1.As(&pAdapter);
			if (FAILED(hr))
			{
				return hr;
			}
			break;
		}
	}
	// Create device !!
	hr = D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_pDevice.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		return hr;
	}
#ifdef _DEBUG
	m_pDevice->SetName(L"Renderer::m_pDevice");
#endif

	// Get a higher feature level
	static const D3D_FEATURE_LEVEL FEATURE_LEVELS[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevels =
	{
		.NumFeatureLevels = ARRAYSIZE(FEATURE_LEVELS),
		.pFeatureLevelsRequested = FEATURE_LEVELS,
		.MaxSupportedFeatureLevel = D3D_FEATURE_LEVEL_11_0
	};

	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	hr = m_pDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevels, sizeof(featureLevels));
	if (SUCCEEDED(hr))
	{
		featureLevel = featureLevels.MaxSupportedFeatureLevel;
	}

	// Create Command Queue
	D3D12_COMMAND_QUEUE_DESC queueDesc =
	{
		.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
		.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
		.NodeMask = 0,
	};

	hr = m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_pCommandQueue.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		return hr;
	}

	// Create Swap Chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc1 =
	{
		.Width = uWidth,
		.Height = uHeight,
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.Stereo = FALSE,
		.SampleDesc = { .Count = 1 },
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = NUM_FRAME_BUFFERS,
		.Scaling = DXGI_SCALING_STRETCH,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
		.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING,
	};

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDesc =
	{
		.RefreshRate =
		{
			.Numerator = 1,
			.Denominator = 60,
		},
		.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
		.Scaling = DXGI_MODE_SCALING_UNSPECIFIED,
		.Windowed = TRUE,
	};

	ComPtr<IDXGISwapChain1> pSwapChain1;
	hr = pDxgiFactory->CreateSwapChainForHwnd(m_pCommandQueue.Get(), hWnd, &swapChainDesc1, &swapChainFullscreenDesc, nullptr, pSwapChain1.GetAddressOf());
	if (FAILED(hr))
	{
		DXGI_SWAP_CHAIN_DESC swapChainDesc =
		{
			.BufferDesc =
			{
				.Width = uWidth,
				.Height = uHeight,
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			},
			.SampleDesc = { .Count = 1 },
			.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
			.BufferCount = NUM_FRAME_BUFFERS,
			.OutputWindow = hWnd,
#if _DEBUG
			.Windowed = FALSE,
#else
			.Windowed = TRUE,
#endif
			.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
			.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING,
		};

		ComPtr<IDXGISwapChain> pSwapChain;
		hr = pDxgiFactory->CreateSwapChain(m_pCommandQueue.Get(), &swapChainDesc, pSwapChain.GetAddressOf());
		if (FAILED(hr))
		{
			return hr;
		}

		hr = pSwapChain.As(&m_pSwapChain);
		if (FAILED(hr))
		{
			return hr;
		}
	}
	else
	{
		hr = pSwapChain1.As(&m_pSwapChain);
		if (FAILED(hr))
		{
			return hr;
		}
	}
	m_uFrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	hr = pDxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
	if (FAILED(hr))
	{
		return hr;
	}

	// Create descriptor heaps
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc =
	{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = NUM_FRAME_BUFFERS,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	};

	hr = m_pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_pRtvHeap.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		return hr;
	}
	m_uRtvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Create frame resources
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart());

	// Create a RTV for each frame
	for (UINT n = 0; n < NUM_FRAME_BUFFERS; n++)
	{
		hr = m_pSwapChain->GetBuffer(n, IID_PPV_ARGS(&m_apRenderTargets[n]));
		if (FAILED(hr))
		{
			return hr;
		}
		m_pDevice->CreateRenderTargetView(m_apRenderTargets[n].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, m_uRtvDescriptorSize);
	}

	// Create command allocator
	hr = m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_pCommandAllocator.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		return hr;
	}
#ifdef _DEBUG
	m_pCommandAllocator->SetName(L"Renderer::m_pCommandAllocator");
#endif

	// =============loading pipeline completed, start to load assets=============

	// Create the command list
	hr = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(m_pCommandList.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		return hr;
	}
	hr = m_pCommandList->Close();
	if (FAILED(hr))
	{
		return hr;
	}
	
	// Create synchronization objects
	hr = m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		return hr;
	}
	m_uFenceValue = 1;
	
	// Create an event handle to use for frame synchronization
	m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_hFenceEvent == nullptr)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		if (FAILED(hr))
		{
			return hr;
		}
	}

	return S_OK;
}

void Renderer::Render()
{
	// This can only be reset when the associated command lists have finished execution
	// ; use fences to determine GPU progress
	m_pCommandAllocator->Reset();

	// Command list can be reset at any time and must be before re-recording
	m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr);

	// Indicate that the back buffer will be used as a render target
	D3D12_RESOURCE_BARRIER preCopyBarriers1 = CD3DX12_RESOURCE_BARRIER::Transition(
		m_apRenderTargets[m_uFrameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	m_pCommandList->ResourceBarrier(1, &preCopyBarriers1);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart(), m_uFrameIndex, m_uRtvDescriptorSize);

	// Record commands into the command list
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_pCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// Indicate that the back buffer will now be used to present
	D3D12_RESOURCE_BARRIER preCopyBarriers2 = CD3DX12_RESOURCE_BARRIER::Transition(
		m_apRenderTargets[m_uFrameIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	m_pCommandList->ResourceBarrier(1, &preCopyBarriers2);

	// Close the command list to further recording
	m_pCommandList->Close();

	// Execute the command list
	ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame
	m_pSwapChain->Present(1, 0);

	// Signal and increment the fence value
	const UINT64 uFence = m_uFenceValue;
	m_pCommandQueue->Signal(m_pFence.Get(), uFence);
	m_uFenceValue++;

	// Wait until the previous frame is finished
	if (m_pFence->GetCompletedValue() < uFence)
	{
		m_pFence->SetEventOnCompletion(uFence, m_hFenceEvent);
		WaitForSingleObject(m_hFenceEvent, INFINITE);
	}

	m_uFrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
}
