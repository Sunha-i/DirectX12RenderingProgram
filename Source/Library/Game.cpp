#include "Game.h"

HWND g_hWnd = nullptr;
HINSTANCE g_hInstance = nullptr;
LPCWSTR g_pszWindowClassName = L"MyWindowClass";
LPCWSTR g_pszWindowName = L"Dx12 Renderer";

LRESULT WindowProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SIZE:
		break;

	case WM_CLOSE:
		if (MessageBox(hWnd,
			L"Do you really want to quit?",
			L"Dx12 Renderer",
			MB_OKCANCEL) == IDOK)
		{
			DestroyWindow(hWnd);
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

HRESULT InitWindow(_In_ HINSTANCE hInstance, _In_ INT nCmdShow)
{
	// Register the window class
	WNDCLASSEX wcex =
	{
		.cbSize = sizeof(WNDCLASSEX),
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = WindowProc,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = hInstance,
		.hIcon = LoadIcon(hInstance,IDI_APPLICATION),
		.hCursor = LoadCursor(nullptr, IDC_ARROW),
		.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),
		.lpszMenuName = nullptr,
		.lpszClassName = g_pszWindowClassName,
		.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION),
	};
	// Error check
	if (!RegisterClassEx(&wcex))
	{
		DWORD dwError = GetLastError();
		MessageBox(
			nullptr,
			L"Call to RegisterClassEx failed!",
			L"Dx12 Renderer",
			NULL
		);
		if (dwError != ERROR_CLASS_ALREADY_EXISTS)
		{
			return HRESULT_FROM_WIN32(dwError);
		}
		return E_FAIL;
	}

	// Create window
	g_hInstance = hInstance;
	RECT rc = { 0, 0, 800, 600 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	g_hWnd = CreateWindow(
		g_pszWindowClassName,
		g_pszWindowName,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);
	// Error check
	if (!g_hWnd)
	{
		DWORD dwError = GetLastError();
		MessageBox(
			nullptr,
			L"Call to CreateWindow failed!",
			L"Dx12 Renderer",
			NULL
		);
		if (dwError != ERROR_CLASS_ALREADY_EXISTS)
		{
			return HRESULT_FROM_WIN32(dwError);
		}
		return E_FAIL;
	}

	// Show window
	ShowWindow(g_hWnd, nCmdShow);

	return S_OK;
}