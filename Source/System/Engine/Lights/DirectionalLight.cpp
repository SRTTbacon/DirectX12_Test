#include "DirectionalLight.h"

using namespace DirectX;

DirectionalLight::DirectionalLight()
	: m_lightBuffer(LightBuffer())
	, m_lightViewProj(XMMatrixIdentity())
	, m_shadowDistance(100.0f)
	, m_shadowScale(25.0f)
	, m_lightPosition(0.0f, 0.0f, 0.0f)
	, m_pDevice(nullptr)
{
	SetRotation(130.0f, 30.0f, 0.0f);
}

void DirectionalLight::Init(ID3D12Device* pDevice)
{
	m_pDevice = pDevice;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	//ディレクショナルライトの情報
	CD3DX12_RESOURCE_DESC lightBuf = CD3DX12_RESOURCE_DESC::Buffer(sizeof(LightBuffer));
	HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &lightBuf, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_lightConstantBuffer));
	if (FAILED(hr)) {
		printf("ライトバッファの生成に失敗しました。\n");
	}
}

void DirectionalLight::Update()
{
	DirectX::XMFLOAT3 storeVector = GetForward();
	DirectX::XMVECTOR lightDirection = DirectX::XMLoadFloat3(&storeVector);
	//DirectX::XMVECTOR sceneCenter = DirectX::XMVectorSet(m_lightPosition.x, m_lightPosition.y, m_lightPosition.z - m_shadowScale / 2.0f, 0.0f);
	DirectX::XMVECTOR sceneCenter = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

	DirectX::XMVECTOR lightPosition = DirectX::XMVectorMultiplyAdd(lightDirection, XMVectorReplicate(-m_shadowDistance), sceneCenter);
	//DirectX::XMVECTOR lightPosition = sceneCenter;
	DirectX::XMStoreFloat3(&storeVector, lightPosition);

	XMStoreFloat4(&m_lightBuffer.lightDirection, lightDirection);

	m_position = XMFLOAT3(storeVector.x, storeVector.y, storeVector.z);

	m_lightBuffer.ambientColor = XMFLOAT4(0.05f, 0.1f, 0.5f, 1.0f);
	m_lightBuffer.diffuseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	XMMATRIX lightMatrix = GetViewMatrix();
	m_lightViewProj = lightMatrix * XMMatrixOrthographicLH(m_shadowScale, m_shadowScale, 0.01f, m_shadowDistance);

	void* p1;
	HRESULT hr = m_lightConstantBuffer->Map(0, nullptr, &p1);
	if (p1) {
		memcpy(p1, &m_lightBuffer, sizeof(LightBuffer));
		m_lightConstantBuffer->Unmap(0, nullptr);
	}
	if (FAILED(hr)) {
		printf("バッファの更新に失敗しました。エラーコード:%d\n", hr);
	}
}

ID3D12Resource* DirectionalLight::GetLightConstantBuffer() const
{
	return m_lightConstantBuffer.Get();
}
