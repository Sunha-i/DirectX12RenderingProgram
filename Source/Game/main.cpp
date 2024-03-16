#include "Common.h"
#include "Game.h"

INT WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ INT nCmdShow)
{
    if (FAILED(InitWindow(hInstance, nCmdShow)))
    {
        return 0;
    }

    return 0;
}