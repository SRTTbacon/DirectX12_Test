#include "DirectionalLight.h"

using namespace DirectX;

DirectionalLight::DirectionalLight()
	: m_near(0.1f)
	, m_far(500.0f)
	, LightTransform()
{
	SetRotation(50.0f, -30.0f, 0.0f);
}

void DirectionalLight::Update()
{
	DirectX::XMFLOAT3 storeVector = GetForward();
	DirectX::XMVECTOR lightDirection = DirectX::XMLoadFloat3(&storeVector);
	DirectX::XMVECTOR sceneCenter = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

	float shadowDistance = 100.0f;
	DirectX::XMVECTOR lightPosition = DirectX::XMVectorMultiplyAdd(lightDirection, XMVectorReplicate(-shadowDistance), sceneCenter);
	DirectX::XMStoreFloat3(&storeVector, lightPosition);

	SetPosition(storeVector.x, storeVector.y, storeVector.z);

	XMStoreFloat3(&lightBuffer.lightDirection, lightDirection);

	lightBuffer.ambientColor = XMFLOAT3(0.2f, 0.2f, 0.2f);
	lightBuffer.diffuseColor = XMFLOAT3(1.0f, 1.0f, 1.0f);


	XMFLOAT3 lightDirection2 = GetForward();
	lightBuffer.lightDirection = lightDirection2;

	XMMATRIX lightMatrix = GetViewMatrix();
	lightViewProj = lightMatrix * XMMatrixOrthographicLH(40, 40, 1.0f, 100.0f);
}
