#pragma once

#include "Common.h"

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT4 color;
};

struct SceneConstantBuffer
{
	XMFLOAT4 offset;
	float padding[60];		// Padding so the Constant Buffer is 256-byte aligned
};
