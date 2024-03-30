#pragma once

#include "Common.h"

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT4 color;
};

struct ConstantBuffer
{
	XMMATRIX world;
	XMMATRIX view;
	XMMATRIX projection;
};