#include "Common.h"

#include "Window/MainWindow.h"

HRESULT MainWindow::InitWindow(_In_ HINSTANCE hInstance, _In_ INT nCmdShow, _In_ PCWSTR pszWindowName)
{
	RAWINPUTDEVICE rid = {};
	rid.usUsagePage = 0x01;
	rid.usUsage = 0x02;
	rid.dwFlags = 0u;
	rid.hwndTarget = nullptr;

	if (!RegisterRawInputDevices(&rid, 1u, sizeof(rid)))
		return E_FAIL;

	return Initialize(
		hInstance,
		nCmdShow,
		pszWindowName,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		800,
		600
	);
}

PCWSTR MainWindow::GetWindowClassName() const
{
	return L"MyWindowClass";
}

LRESULT MainWindow::HandleMessage(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SIZE:
		break;

	case WM_CLOSE:
		if (MessageBox(m_hWnd,
			L"Do you really want to quit?",
			L"Dx12 Renderer",
			MB_OKCANCEL) == IDOK)
		{
			DestroyWindow(m_hWnd);
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	default:
		return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
	}
    return 0;
}