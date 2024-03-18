#include "Renderer/Renderer.h"

Renderer::Renderer()
	: m_pDevice()
	, m_pCommandQueue()
	, m_pSwapChain()
	, m_pRtvHeap()
	, m_pCbvHeap()
	, m_pRootSignature()
	, m_pPipelineState()
	, m_pCommandList()
	, m_uRtvDescriptorSize(0)
	, m_uFrameIndex(0)
	, m_hFenceEvent()
	, m_uFenceValue(0)
	, m_pVertexBuffer()
	, m_pConstantBuffer()
	, m_vertexBufferView()
	, m_constantBufferData()
	, m_upCbvDataBegin()
{
}

HRESULT Renderer::InitDevice(_In_ HWND hWnd)
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(hWnd, &rc);
	UINT uWidth = static_cast<UINT>(rc.right - rc.left);
	UINT uHeight = static_cast<UINT>(rc.bottom - rc.top);

	// Setup the viewport
	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;
	m_viewport.Width = static_cast<FLOAT>(uWidth);
	m_viewport.Height = static_cast<FLOAT>(uHeight);

	// Set scissorRect
	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = static_cast<LONG>(uWidth);
	m_scissorRect.bottom = static_cast<LONG>(uHeight);

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
	{
		// Describe and create a render target view (RTV) descriptor heap
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

		// Describe and create a constant buffer view (CBV) descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc =
		{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = 1,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		};

		hr = m_pDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(m_pCbvHeap.ReleaseAndGetAddressOf()));
		if (FAILED(hr))
		{
			return hr;
		}
	}

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


	// Create a root signature consisting of a descriptor table with a single CBV
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

		if (FAILED(m_pDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
		CD3DX12_ROOT_PARAMETER1 rootParameters[1];

		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error);
		if (FAILED(hr))
		{
			return hr;
		}
		hr = m_pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(m_pRootSignature.ReleaseAndGetAddressOf()));
		if (FAILED(hr))
		{
			return hr;
		}
#ifdef _DEBUG
		m_pRootSignature->SetName(L"Renderer::m_pRootSignature");
#endif
	}

	// Read the compiled shaders
	ComPtr<ID3DBlob> pVertexShader;
	ComPtr<ID3DBlob> pPixelShader;
	{
#if defined(_DEBUG)
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		hr = D3DCompileFromFile(L"../Library/Shader.fx", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, pVertexShader.GetAddressOf(), nullptr);
		if (FAILED(hr))
		{
			return hr;
		}
		hr = D3DCompileFromFile(L"../Library/Shader.fx", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, pPixelShader.GetAddressOf(), nullptr);
		if (FAILED(hr))
		{
			return hr;
		}
	}

	// Define the vertex input layout
	D3D12_INPUT_ELEMENT_DESC aInputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Create a PSO description, then create the object
	{
		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc =
		{
			.pRootSignature = m_pRootSignature.Get(),
			.VS = CD3DX12_SHADER_BYTECODE(pVertexShader.Get()),
			.PS = CD3DX12_SHADER_BYTECODE(pPixelShader.Get()),
			.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
			.SampleMask = UINT_MAX,
			.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
			.DepthStencilState =
			{
				.DepthEnable = FALSE,
				.StencilEnable = FALSE
			},
			.InputLayout = { aInputElementDescs, ARRAYSIZE(aInputElementDescs) },
			.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			.NumRenderTargets = 1,
			.RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM, },
			.SampleDesc =
			{
				.Count = 1,
			},
		};
		hr = m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pPipelineState.ReleaseAndGetAddressOf()));
		if (FAILED(hr))
		{
			return hr;
		}
#ifdef _DEBUG
		m_pPipelineState->SetName(L"Renderer::m_pPipelineState");
#endif
	}

	// Create the command list
	hr = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator.Get(), m_pPipelineState.Get(), IID_PPV_ARGS(m_pCommandList.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		return hr;
	}
	hr = m_pCommandList->Close();
	if (FAILED(hr))
	{
		return hr;
	}

	// Create the vertex buffer
	{
		Vertex triangleVertices[] =
		{
			{ { 0.0f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
			{ { 0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
			{ { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
		};

		const UINT vertexBufferSize = sizeof(triangleVertices);

		CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
		hr = m_pDevice->CreateCommittedResource(
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

		// Copy the triangle data to the vertex buffer
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);
		hr = m_pVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
		if (FAILED(hr))
		{
			return hr;
		}
		memcpy(pVertexDataBegin, triangleVertices, vertexBufferSize);
		m_pVertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view
		m_vertexBufferView =
		{
			.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress(),
			.SizeInBytes = vertexBufferSize,
			.StrideInBytes = sizeof(Vertex),
		};
	}

	// Create the constant buffer
	{
		const UINT uConstantBufferSize = sizeof(SceneConstantBuffer);

		CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uConstantBufferSize);
		hr = m_pDevice->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_pConstantBuffer)
		);
		if (FAILED(hr))
		{
			return hr;
		}

		// Describe and create a constant buffer view
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc =
		{
			.BufferLocation = m_pConstantBuffer->GetGPUVirtualAddress(),
			.SizeInBytes = uConstantBufferSize
		};
		m_pDevice->CreateConstantBufferView(&cbvDesc, m_pCbvHeap->GetCPUDescriptorHandleForHeapStart());

		// Map and initialize
		CD3DX12_RANGE readRange(0, 0);
		hr = m_pConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_upCbvDataBegin));
		if (FAILED(hr))
		{
			return hr;
		}
		memcpy(m_upCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
	}

	// Create synchronization objects
	{
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

		// Wait until assets have been uploaded to the GPU
		WaitForPreviousFrame();
	}
	
	return S_OK;
}

void Renderer::Update()
{
	const float translationSpeed = 0.005f;
	const float offsetBounds = 1.25f;

	m_constantBufferData.offset.x += translationSpeed;
	if (m_constantBufferData.offset.x > offsetBounds)
	{
		m_constantBufferData.offset.x = -offsetBounds;
	}
	memcpy(m_upCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
}

void Renderer::Render()
{
	// This can only be reset when the associated command lists have finished execution
	// ; use fences to determine GPU progress
	m_pCommandAllocator->Reset();

	// Command list can be reset at any time and must be before re-recording
	m_pCommandList->Reset(m_pCommandAllocator.Get(), m_pPipelineState.Get());

	// Set necessary state
	m_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { m_pCbvHeap.Get() };
	m_pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	m_pCommandList->SetGraphicsRootDescriptorTable(0, m_pCbvHeap->GetGPUDescriptorHandleForHeapStart());

	m_pCommandList->RSSetViewports(1, &m_viewport);
	m_pCommandList->RSSetScissorRects(1, &m_scissorRect);

	// Indicate that the back buffer will be used as a render target
	D3D12_RESOURCE_BARRIER preCopyBarriers1 = CD3DX12_RESOURCE_BARRIER::Transition(
		m_apRenderTargets[m_uFrameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	m_pCommandList->ResourceBarrier(1, &preCopyBarriers1);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart(), m_uFrameIndex, m_uRtvDescriptorSize);
	m_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Record commands into the command list
	const float clearColor[] = { 0.6f, 0.7f, 0.8f, 1.0f };
	m_pCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_pCommandList->DrawInstanced(3, 1, 0, 0);

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

	WaitForPreviousFrame();
}

void Renderer::WaitForPreviousFrame()
{
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
