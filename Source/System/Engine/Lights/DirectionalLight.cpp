#include "DirectionalLight.h"

using namespace DirectX;

DirectionalLight::DirectionalLight()
	: LightTransform()
	, m_lightBuffer(LightBuffer())
	, m_lightViewProj(XMMatrixIdentity())
	, m_shadowDistance(100.0f)
{
	SetRotation(50.0f, -30.0f, 0.0f);
}

void DirectionalLight::Update()
{
	DirectX::XMFLOAT3 storeVector = GetForward();
	DirectX::XMVECTOR lightDirection = DirectX::XMLoadFloat3(&storeVector);
	DirectX::XMVECTOR sceneCenter = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

	DirectX::XMVECTOR lightPosition = DirectX::XMVectorMultiplyAdd(lightDirection, XMVectorReplicate(-m_shadowDistance), sceneCenter);
	DirectX::XMStoreFloat3(&storeVector, lightPosition);

	SetPosition(storeVector.x, storeVector.y, storeVector.z);

	XMStoreFloat4(&m_lightBuffer.lightDirection, lightDirection);

	m_lightBuffer.ambientColor = XMFLOAT4(0.05f, 0.1f, 0.5f, 1.0f);
	m_lightBuffer.diffuseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	XMMATRIX lightMatrix = GetViewMatrix();
	m_lightViewProj = lightMatrix * XMMatrixOrthographicLH(50, 50, 0.1f, m_shadowDistance);
}
