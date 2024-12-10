#pragma once
#include <DirectXMath.h>
#include "LightTransform.h"

struct LightBuffer
{
	DirectX::XMFLOAT3 lightDirection;
	DirectX::XMFLOAT3 ambientColor;
	DirectX::XMFLOAT3 diffuseColor;
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