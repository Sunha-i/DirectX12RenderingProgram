#pragma once

#include "Texture/Texture.h"

class Material
{
public:
	Material();
	~Material() = default;

	std::shared_ptr<Texture> pDiffuse;
	std::shared_ptr<Texture> pSpecular;

	XMFLOAT3 AmbientColor;
	XMFLOAT3 DiffuseColor;
	XMFLOAT3 SpecularColor;
};