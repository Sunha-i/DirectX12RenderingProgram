#include "Light/PointLight.h"

PointLight::PointLight(_In_ const XMFLOAT4& position, _In_ const XMFLOAT4& color)
	: m_vPosition(position)
	, m_vColor(color)
{
}

const XMFLOAT4& PointLight::GetPosition() const
{
	return m_vPosition;
}

const XMFLOAT4& PointLight::GetColor() const
{
	return m_vColor;
}

void PointLight::Update(_In_ FLOAT deltaTime)
{
}