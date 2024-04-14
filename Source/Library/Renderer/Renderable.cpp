#include "Renderable.h"

Renderable::Renderable(_In_ const XMFLOAT4& outputColor)
	: m_vOutputColor(outputColor)
	, m_mWorld(XMMatrixIdentity())
{
}

const XMMATRIX& Renderable::GetWorldMatrix() const
{
	return m_mWorld;
}

const XMFLOAT4& Renderable::GetOutputColor() const
{
	return m_vOutputColor;
}

void Renderable::RotateX(_In_ FLOAT angle)
{
	m_mWorld *= XMMatrixRotationX(angle);
}

void Renderable::RotateY(_In_ FLOAT angle)
{
	m_mWorld *= XMMatrixRotationY(angle);
}

void Renderable::RotateZ(_In_ FLOAT angle)
{
	m_mWorld *= XMMatrixRotationZ(angle);
}

void Renderable::RotateRollPitchYaw(_In_ FLOAT roll, _In_ FLOAT pitch, _In_ FLOAT yaw)
{
	m_mWorld *= XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
}

void Renderable::Scale(_In_ FLOAT scaleX, _In_ FLOAT scaleY, _In_ FLOAT scaleZ)
{
	m_mWorld *= XMMatrixScaling(scaleX, scaleY, scaleZ);
}

void Renderable::Translate(_In_ const XMVECTOR& offset)
{
	m_mWorld *= XMMatrixTranslationFromVector(offset);
}