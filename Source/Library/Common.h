#pragma once

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")
 
#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <directxcolors.h>

#include <wrl.h>
#include <dxgi1_6.h>

#include <filesystem>
#include <unordered_map>

#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_ConvertToLeftHanded)

using namespace Microsoft::WRL;
using namespace DirectX;