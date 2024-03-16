#pragma once

#include "Common.h"

LRESULT CALLBACK WindowProc
(
	_In_ HWND hWnd,
	_In_ UINT uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
);

HRESULT InitWindow(_In_ HINSTANCE hInstance, _In_ INT nCmdShow);