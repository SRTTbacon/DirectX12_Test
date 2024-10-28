#pragma once
#include <DirectXMath.h>

struct LightBuffer
{
	DirectX::XMFLOAT3 lightDirection;
	DirectX::XMFLOAT4 ambientColor;
	DirectX::XMFLOAT4 diffuseColor;
	DirectX::XMFLOAT4 specularColor;
};

struct DirectionalLight
{
	DirectX::XMMATRIX lightView;
	DirectX::XMMATRIX lightProj;;
	DirectX::XMMATRIX lightViewProj;

	LightBuffer lightBuffer;

	DirectionalLight();
};