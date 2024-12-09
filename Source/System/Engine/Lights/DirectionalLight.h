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
	DirectX::XMMATRIX lightViewProj;

	LightBuffer lightBuffer;

	float m_near;
	float m_far;

	DirectionalLight();

	void Update();
};