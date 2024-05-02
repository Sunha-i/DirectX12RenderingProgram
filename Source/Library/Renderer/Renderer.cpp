#include "Renderer/Renderer.h"

Renderer::Renderer()
	: m_pDevice()
	, m_pCommandQueue()
	, m_pSwapChain()
	, m_pRtvHeap()
	, m_pDsvHeap()
	, m_pSrvHeap()
	, m_pRootSignature()
	, m_pLambertPipelineState()
	, m_pSolidPipelineState()
	, m_pCommandList()
	, m_uRtvDescriptorSize(0)
	, m_uFrameIndex(0)
	, m_hFenceEvent()
	, m_pVertexBuffer()
	, m_pIndexBuffer()
	, m_pTextureResource()
	, m_vertexBufferView()
	, m_indexBufferView()
	, m_worldMatrix()
	, m_viewMatrix()
	, m_projectionMatrix()
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

	// Initialize the world matrix
	m_worldMatrix = XMMatrixIdentity();

	// Initialize the view matrix
	static const XMVECTOR eye = XMVectorSet(0.0f, 3.0f, -10.0f, 0.0f);
	static const XMVECTOR at = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	static const XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_viewMatrix = XMMatrixLookAtLH(eye, at, up);

	// Initialize the projection matrix
	m_projectionMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV4, uWidth / (FLOAT)uHeight, 0.01f, 100.0f);

	// Initialize the scene output color
	m_vOutputColor = XMFLOAT4(0, 0, 0, 0);

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

		// Describe and create a depth stencil view (DSV) descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc =
		{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
			.NumDescriptors = 1,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		};
		hr = m_pDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(m_pDsvHeap.ReleaseAndGetAddressOf()));
		if (FAILED(hr))
		{
			return hr;
		}

		// Describe and create a shader resource view (SRV) heap for the texture
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc =
		{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = 1,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		};
		hr = m_pDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(m_pSrvHeap.ReleaseAndGetAddressOf()));
		if (FAILED(hr))
		{
			return hr;
		}
	}

	// Create frame resources
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV & a command allocator for each frame
		for (UINT uBackBufferIdx = 0; uBackBufferIdx < NUM_FRAME_BUFFERS; ++uBackBufferIdx)
		{
			hr = m_pSwapChain->GetBuffer(uBackBufferIdx, IID_PPV_ARGS(&m_apRenderTargets[uBackBufferIdx]));
			if (FAILED(hr))
			{
				return hr;
			}
			m_pDevice->CreateRenderTargetView(m_apRenderTargets[uBackBufferIdx].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_uRtvDescriptorSize);

			hr = m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_apCommandAllocators[uBackBufferIdx].ReleaseAndGetAddressOf()));
			if (FAILED(hr))
			{
				return hr;
			}
#ifdef _DEBUG
			WCHAR szCommandAllocatorName[64] = { L'\0',};
			swprintf_s(szCommandAllocatorName, L"Renderer::m_apCommandAllocators[%u]", uBackBufferIdx);

			m_apCommandAllocators[uBackBufferIdx]->SetName(szCommandAllocatorName);
#endif
		}
	}

	// Create the depth stencil view
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc =
		{
			.Format = DXGI_FORMAT_D32_FLOAT,
			.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
			.Flags = D3D12_DSV_FLAG_NONE,
		};
		const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
		const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, uWidth, uHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);
		const CD3DX12_CLEAR_VALUE clearVal(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);
		hr = m_pDevice->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearVal,
			IID_PPV_ARGS(m_pDepthStencil.ReleaseAndGetAddressOf())
		);
		if (FAILED(hr))
		{
			return hr;
		}
		m_pDevice->CreateDepthStencilView(m_pDepthStencil.Get(), &depthStencilDesc, m_pDsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

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
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

		CD3DX12_ROOT_PARAMETER1 rootParameters[2];
		rootParameters[0].InitAsConstantBufferView(0, 0);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

		D3D12_STATIC_SAMPLER_DESC staticSampler =
		{
			.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
			.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			.MipLODBias = 0,
			.MaxAnisotropy = 0,
			.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
			.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
			.MinLOD = 0.0f,
			.MaxLOD = D3D12_FLOAT32_MAX,
			.ShaderRegister = 0,
			.RegisterSpace = 0,
			.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
		};

		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &staticSampler, rootSignatureFlags);

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

	// Create the constant buffer memory and map the resource
	{
		const D3D12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
		size_t cbSize = NUM_DRAW_CALLS * NUM_FRAME_BUFFERS * sizeof(ConstantBuffer);

		const D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);
		hr = m_pDevice->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_pPerFrameConstants.ReleaseAndGetAddressOf())
		);
		if (FAILED(hr))
		{
			return hr;
		}
		hr = m_pPerFrameConstants->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedConstantData));
		if (FAILED(hr))
		{
			return hr;
		}

		// GPU virtual address of the resource
		m_constantDataGpuAddr = m_pPerFrameConstants->GetGPUVirtualAddress();
	}

	// Read the compiled shaders
	ComPtr<ID3DBlob> pTriangleVS;
	ComPtr<ID3DBlob> pLambertPS;
	ComPtr<ID3DBlob> pSolidColorPS;
	{
#if defined(_DEBUG)
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		hr = D3DCompileFromFile(L"../Library/Shader.fx", nullptr, nullptr, "VSTriangle", "vs_5_0", compileFlags, 0, pTriangleVS.GetAddressOf(), nullptr);
		if (FAILED(hr))
		{
			return hr;
		}
		hr = D3DCompileFromFile(L"../Library/Shader.fx", nullptr, nullptr, "PSLambert", "ps_5_0", compileFlags, 0, pLambertPS.GetAddressOf(), nullptr);
		if (FAILED(hr))
		{
			return hr;
		}
		hr = D3DCompileFromFile(L"../Library/Shader.fx", nullptr, nullptr, "PSSolid", "ps_5_0", compileFlags, 0, pSolidColorPS.GetAddressOf(), nullptr);
		if (FAILED(hr))
		{
			return hr;
		}
	}

	// Define the vertex input layout
	D3D12_INPUT_ELEMENT_DESC aInputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Describe and create the graphics pipeline state object (PSO).
	{
		// Create a PSO for the Lambert pixel shader
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc =
		{
			.pRootSignature = m_pRootSignature.Get(),
			.VS = CD3DX12_SHADER_BYTECODE(pTriangleVS.Get()),
			.PS = CD3DX12_SHADER_BYTECODE(pLambertPS.Get()),
			.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
			.SampleMask = UINT_MAX,
			.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
			.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
			.InputLayout = { aInputElementDescs, ARRAYSIZE(aInputElementDescs) },
			.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			.NumRenderTargets = 1,
			.RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM, },
			.DSVFormat = DXGI_FORMAT_D32_FLOAT,
			.SampleDesc = { .Count = 1 },
		};
		hr = m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pLambertPipelineState.ReleaseAndGetAddressOf()));
		if (FAILED(hr))
		{
			return hr;
		}
#ifdef _DEBUG
		m_pLambertPipelineState->SetName(L"Renderer::m_pLambertPipelineState");
#endif
	}{
		// Create the PSO for the solid color pixel shader
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc =
		{
			.pRootSignature = m_pRootSignature.Get(),
			.VS = CD3DX12_SHADER_BYTECODE(pTriangleVS.Get()),
			.PS = CD3DX12_SHADER_BYTECODE(pSolidColorPS.Get()),
			.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
			.SampleMask = UINT_MAX,
			.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
			.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
			.InputLayout = { aInputElementDescs, ARRAYSIZE(aInputElementDescs) },
			.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			.NumRenderTargets = 1,
			.RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM, },
			.DSVFormat = DXGI_FORMAT_D32_FLOAT,
			.SampleDesc = { .Count = 1 },
		};
		hr = m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pSolidPipelineState.ReleaseAndGetAddressOf()));
		if (FAILED(hr))
		{
			return hr;
		}
#ifdef _DEBUG
		m_pSolidPipelineState->SetName(L"Renderer::m_pSolidPipelineState");
#endif
	}

	// Create the command list
	hr = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_apCommandAllocators[m_uFrameIndex].Get(), nullptr, IID_PPV_ARGS(m_pCommandList.ReleaseAndGetAddressOf()));
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
		Vertex cubeVertices[] =
		{
			{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },

			{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
			{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
			{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },

			{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },

			{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },

			{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
			{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
			{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
			{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },

			{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		};
		const UINT vertexBufferSize = sizeof(cubeVertices);

		// Create committed resource for vertex buffer
		const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
		const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
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

		// Copy the cube data to the vertex buffer
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);
		hr = m_pVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
		if (FAILED(hr))
		{
			return hr;
		}
		memcpy(pVertexDataBegin, cubeVertices, vertexBufferSize);
		m_pVertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view
		m_vertexBufferView =
		{
			.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress(),
			.SizeInBytes = vertexBufferSize,
			.StrideInBytes = sizeof(Vertex),
		};
	}

	// Create the index buffer
	{
		static WORD cubeIndices[] =
		{	
			 3,  1,  0,  2,  1,  3,		// TOP
			 6,  4,  5,  7,  4,  6,		// BOTTOM
			11,  9,  8, 10,  9, 11,		// LEFT
			14, 12, 13, 15, 12, 14,		// RIGHT
			19, 17, 16, 18, 17, 19,		// FRONT
			22, 20, 21, 23, 20, 22,		// BACK
		};
		const UINT indexBufferSize = sizeof(cubeIndices);

		// Create committed resource for index buffer
		const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
		const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
		hr = m_pDevice->CreateCommittedResource(
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
		memcpy(pIndexDataBegin, cubeIndices, indexBufferSize);
		m_pIndexBuffer->Unmap(0, nullptr);

		// Initialize the index buffer view
		m_indexBufferView =
		{
			.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress(),
			.SizeInBytes = indexBufferSize,
			.Format = DXGI_FORMAT_R16_UINT,
		};
	}

	// Loads a texture, upload resources to the GPU
	{
		// start recording upload commands
		ResourceUploadBatch resourceUpload(m_pDevice.Get());
		resourceUpload.Begin();

		m_pTextureResource.Reset();
		hr = CreateDDSTextureFromFile(m_pDevice.Get(), resourceUpload, L"../Library/seafloor.dds", m_pTextureResource.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			return hr;
		}

		// upload and wait for the upload thread to terminate
		auto uploadFin = resourceUpload.End(m_pCommandQueue.Get());
		uploadFin.wait();

		// describe and create a SRV for the texture
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc =
		{
			.Format = m_pTextureResource->GetDesc().Format,
			.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Texture2D = { .MipLevels = 1 }
		};
		m_pDevice->CreateShaderResourceView(m_pTextureResource.Get(), &srvDesc, m_pSrvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	// Create synchronization objects
	{
		hr = m_pDevice->CreateFence(m_auFenceValues[m_uFrameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.ReleaseAndGetAddressOf()));
		if (FAILED(hr))
		{
			return hr;
		}
		m_auFenceValues[m_uFrameIndex]++;

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

		// Wait for pending GPU work to complete
		WaitForGpu();
	}
	
	return S_OK;
}

HRESULT Renderer::AddRenderable(_In_ PCWSTR pszRenderableName, _In_ const std::shared_ptr<Renderable>& renderable)
{
	if (m_umRenderables.contains(pszRenderableName))
	{
		return E_FAIL;
	}
	m_umRenderables[pszRenderableName] = renderable;

	return S_OK;
}

HRESULT Renderer::AddPointLight(_In_ size_t index, _In_ const std::shared_ptr<PointLight>& pPointLight)
{
	if (index >= NUM_LIGHTS)
	{
		return E_FAIL;
	}
	m_apPointLights[index] = pPointLight;

	return S_OK;
}

void Renderer::Update(_In_ FLOAT deltaTime)
{
	static FLOAT t = 0.0f;
	t += deltaTime;

	// Rotate the cube around the Y-axis
	m_worldMatrix = XMMatrixRotationY(t);

	for (auto it = m_umRenderables.begin(); it != m_umRenderables.end(); ++it)
	{
		it->second->Update(deltaTime);
	}

	for (UINT i = 0; i < NUM_LIGHTS; ++i)
	{
		m_apPointLights[i]->Update(deltaTime);
	}
}

void Renderer::Render()
{
	// This can only be reset when the associated command lists have finished execution
	// ; use fences to determine GPU progress
	m_apCommandAllocators[m_uFrameIndex]->Reset();

	// Command list can be reset at any time and must be before re-recording
	m_pCommandList->Reset(m_apCommandAllocators[m_uFrameIndex].Get(), m_pLambertPipelineState.Get());

	// Set the graphics RS to be used by this frame
	m_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { m_pSrvHeap.Get() };
	m_pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	m_pCommandList->SetGraphicsRootDescriptorTable(1, m_pSrvHeap->GetGPUDescriptorHandleForHeapStart());
	m_pCommandList->RSSetViewports(1, &m_viewport);
	m_pCommandList->RSSetScissorRects(1, &m_scissorRect);

	// Index into the available cbs based on the number of draw calls
	unsigned int constantBufferIndex = NUM_DRAW_CALLS * (m_uFrameIndex % NUM_FRAME_BUFFERS);
	ConstantBuffer cb = {};

	// Shaders compiled with default transformation matrices
	cb.World = XMMatrixTranspose(m_worldMatrix);
	cb.View = XMMatrixTranspose(m_viewMatrix);
	cb.Projection = XMMatrixTranspose(m_projectionMatrix);

	for (size_t i = 0; i < NUM_LIGHTS; ++i)
	{
		cb.LightDirs[i] = m_apPointLights[i]->GetPosition();
		cb.LightColors[i] = m_apPointLights[i]->GetColor();
	}
	cb.OutputColor = m_vOutputColor;

	// Set the constants for the first draw call
	memcpy(&m_mappedConstantData[constantBufferIndex], &cb, sizeof(ConstantBuffer));

	// Bind the constants to the shader
	auto baseGpuAddress = m_constantDataGpuAddr + sizeof(ConstantBuffer) * constantBufferIndex;
	m_pCommandList->SetGraphicsRootConstantBufferView(0, baseGpuAddress);

	// Indicate that the back buffer will be used as a render target
	D3D12_RESOURCE_BARRIER preCopyBarriers1 = CD3DX12_RESOURCE_BARRIER::Transition(
		m_apRenderTargets[m_uFrameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	m_pCommandList->ResourceBarrier(1, &preCopyBarriers1);

	// Set render target and depth buffer in OM stage
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart(), m_uFrameIndex, m_uRtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pDsvHeap->GetCPUDescriptorHandleForHeapStart());
	m_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// Clear render target & depth buffer
	const float clearColor[] = { 0.13f, 0.13f, 0.13f, 1.0f };
	m_pCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_pCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// Set up the input assembler
	m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_pCommandList->IASetIndexBuffer(&m_indexBufferView);

	// Draw the first cube
	m_pCommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
	baseGpuAddress += sizeof(ConstantBuffer);
	++constantBufferIndex;

	// Render each light
	m_pCommandList->SetPipelineState(m_pSolidPipelineState.Get());

	for (auto it = m_umRenderables.begin(); it != m_umRenderables.end(); ++it)
	{
		// Update the world variable to reflect the current light
		cb.World = XMMatrixTranspose(it->second->GetWorldMatrix());
		cb.OutputColor = it->second->GetOutputColor();

		// Set the constants for the draw call
		memcpy(&m_mappedConstantData[constantBufferIndex], &cb, sizeof(ConstantBuffer));

		// Bind the constants to the shader
		m_pCommandList->SetGraphicsRootConstantBufferView(0, baseGpuAddress);

		// Draw the second cube
		m_pCommandList->DrawIndexedInstanced(it->second->GetNumIndices(), 1, 0, 0, 0);
		baseGpuAddress += sizeof(ConstantBuffer);
		++constantBufferIndex;
	}

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

	// Prepare to render the next frame
	MoveToNextFrame();
}

void Renderer::WaitForGpu()
{
	// Schedule a Signal command
	const UINT64 uFence = m_auFenceValues[m_uFrameIndex];
	m_pCommandQueue->Signal(m_pFence.Get(), uFence);

	// Wait until the fence has been processed
	m_pFence->SetEventOnCompletion(uFence, m_hFenceEvent);
	WaitForSingleObject(m_hFenceEvent, INFINITE);

	// Increment the value for current frame
	m_auFenceValues[m_uFrameIndex]++;
}

void Renderer::MoveToNextFrame()
{
	// Schedule a Signal command
	const UINT64 uFence = m_auFenceValues[m_uFrameIndex];
	m_pCommandQueue->Signal(m_pFence.Get(), uFence);

	// Update the frame index
	m_uFrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// Wait until the next frame is ready to be rendered
	if (m_pFence->GetCompletedValue() < uFence)
	{
		m_pFence->SetEventOnCompletion(uFence, m_hFenceEvent);
		WaitForSingleObject(m_hFenceEvent, INFINITE);
	}

	// Set the value for the next frame
	m_auFenceValues[m_uFrameIndex]++;
}
