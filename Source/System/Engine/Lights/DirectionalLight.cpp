#include "DirectionalLight.h"

using namespace DirectX;

void DirectionalLight::SetPosition(XMFLOAT3 position)
{
	XMVECTOR lightPos = XMVectorSet(position.x, position.y, position.z, 0.0f);
	XMVECTOR target = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR upFoward = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	lightView = XMMatrixLookAtLH(lightPos, target, upFoward);
	lightProj = XMMatrixOrthographicLH(100.0f, 100.0f, 0.1f, 500.0f);
	lightViewProj = XMMatrixMultiply(lightView, lightProj);
}
