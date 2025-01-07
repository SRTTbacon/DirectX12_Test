#pragma once
#include <DirectXMath.h>
#include "LightTransform.h"

struct LightBuffer
{
	DirectX::XMFLOAT4 lightDirection;
	DirectX::XMFLOAT4 ambientColor;
	DirectX::XMFLOAT4 diffuseColor;
};

class DirectionalLight : public LightTransform
{
public:
	DirectX::XMMATRIX m_lightViewProj;

	LightBuffer m_lightBuffer;

	float m_shadowDistance;

	DirectionalLight();

	void Update();
};