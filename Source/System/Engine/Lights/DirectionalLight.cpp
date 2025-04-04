#include "DirectionalLight.h"

using namespace DirectX;

const float DirectionalLight::SHADOW_GRID_SIZE = 1.0f; //影の安定化のためのグリッドサイズ

DirectionalLight::DirectionalLight()
	: m_lightBuffer(LightBuffer())
	, m_lightViewProj(XMMatrixIdentity())
	, m_shadowDistance(50.0f)
	, m_shadowScale(75.0f)
	, m_lightPosition(0.0f, 0.0f, 0.0f)
	, m_pDevice(nullptr)
{
	SetRotation(50.0f, -30.0f, 0.0f);
	m_lightPosition.x = m_shadowScale / 4.0f;
	m_lightPosition.y = 10.0f;
	m_lightPosition.z = m_shadowScale / 4.0f;

	m_lightBuffer.fogColor = XMFLOAT4(0.5f, 0.7f, 1.0f, 1.0f);
	m_lightBuffer.fogStartEnd = XMFLOAT2(50.0f, 200.0f);
}

void DirectionalLight::Init(ID3D12Device* pDevice)
{
	m_pDevice = pDevice;

	m_lightBuffer.ambientColor = XMFLOAT4(0.05f, 0.1f, 0.5f, 1.0f);
	m_lightBuffer.diffuseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
}

ComPtr<ID3D12Resource> DirectionalLight::CreateConstantBuffer()
{
	ComPtr<ID3D12Resource> buffer;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	//ディレクショナルライトの情報
	CD3DX12_RESOURCE_DESC lightBuf = CD3DX12_RESOURCE_DESC::Buffer(sizeof(LightBuffer));
	HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &lightBuf, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer));
	if (FAILED(hr)) {
		printf("ライトバッファの生成に失敗しました。\n");
	}

	return buffer;
}

void DirectionalLight::LateUpdate()
{
	DirectX::XMFLOAT3 storeVector = GetForward();
	DirectX::XMVECTOR lightDirection = DirectX::XMLoadFloat3(&storeVector);
	XMFLOAT3 temp = m_lightPosition;
	temp.x = round(m_lightPosition.x / SHADOW_GRID_SIZE) * SHADOW_GRID_SIZE;
	temp.z = round(m_lightPosition.z / SHADOW_GRID_SIZE) * SHADOW_GRID_SIZE;
	DirectX::XMVECTOR sceneCenter = DirectX::XMVectorSet(temp.x, temp.y, temp.z, 1.0f);

	DirectX::XMVECTOR lightPosition = DirectX::XMVectorMultiplyAdd(lightDirection, XMVectorReplicate(-m_shadowDistance / 2.0f), sceneCenter);
	DirectX::XMStoreFloat3(&storeVector, lightPosition);

	XMStoreFloat4(&m_lightBuffer.lightDirection, lightDirection);
	m_position = storeVector;

	XMMATRIX lightMatrix = GetViewMatrix();
	m_lightViewProj = lightMatrix * XMMatrixOrthographicLH(m_shadowScale, m_shadowScale, 0.1f, m_shadowDistance);
}

void DirectionalLight::MemCopyBuffer(void* p) const
{
	if (p) {
		memcpy(p, &m_lightBuffer, sizeof(LightBuffer));
	}
}
