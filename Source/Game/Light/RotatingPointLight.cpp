#include "Light/RotatingPointLight.h"

RotatingPointLight::RotatingPointLight(_In_ const XMFLOAT4& position, _In_ const XMFLOAT4& color)
	: PointLight(position, color)
{
}

void RotatingPointLight::Update(_In_ FLOAT deltaTime)
{
	// Rotate around the origin
	XMMATRIX rotate = XMMatrixRotationY(-1.5f * deltaTime);
	XMVECTOR position = XMLoadFloat4(&m_vPosition);
	position = XMVector3Transform(position, rotate);
	XMStoreFloat4(&m_vPosition, position);
}