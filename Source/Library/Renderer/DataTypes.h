#pragma once

#define NUM_LIGHTS 2

#include "Common.h"

struct Vertex
{
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
};

struct ConstantBuffer
{
	XMMATRIX World;
	XMMATRIX View;
	XMMATRIX Projection;
	XMFLOAT4 LightDirs[2];
	XMFLOAT4 LightColors[2];
	XMFLOAT4 OutputColor;
};