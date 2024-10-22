#pragma once
#include <DirectXMath.h>

struct DirectionalLight
{
	DirectX::XMMATRIX lightView;
	DirectX::XMMATRIX lightProj;;
	DirectX::XMMATRIX lightViewProj;
	DirectX::XMFLOAT3 lightDirection;

	void SetPosition(DirectX::XMFLOAT3 position);
};