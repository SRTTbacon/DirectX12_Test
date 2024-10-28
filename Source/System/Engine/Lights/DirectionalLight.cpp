#include "DirectionalLight.h"

using namespace DirectX;

DirectionalLight::DirectionalLight()
{
	XMVECTOR lightPos = XMVectorSet(1.0f, 5.0f, 1.0f, 0.0f);
	XMVECTOR target = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR upFoward = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	lightView = XMMatrixLookAtLH(lightPos, target, upFoward);
	lightProj = XMMatrixOrthographicLH(100.0f, 100.0f, 0.1f, 500.0f);
	lightViewProj = XMMatrixMultiply(lightView, lightProj);

	lightBuffer.lightDirection = XMFLOAT3(-1.0f, -1.0f, -1.0f);
	lightBuffer.ambientColor = XMFLOAT4(0.1f, 0.1f, 0.5f, 1.0f);
	lightBuffer.diffuseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	lightBuffer.specularColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
}
