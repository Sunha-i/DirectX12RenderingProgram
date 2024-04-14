#include "BaseCube.h"

BaseCube::BaseCube(_In_ const XMFLOAT4& outputColor)
	: Renderable(outputColor)
{
}

UINT BaseCube::GetNumVertices() const
{
	return NUM_VERTICES;
}

UINT BaseCube::GetNumIndices() const
{
	return NUM_INDICES;
}

const Vertex* BaseCube::getVertices() const
{
	return VERTICES;
}

const WORD* BaseCube::getIndices() const
{
	return INDICES;
}