#include "Game.h"

namespace library
{
    /*--------------------------------------------------------------------
      Function definitions
    --------------------------------------------------------------------*/
    void PrintHi()
    {
        OutputDebugString(L"hi\n");
        MessageBox(nullptr, L"hi", L"Dx12 Rendering Program", MB_OK);
    }
}