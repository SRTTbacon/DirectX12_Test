#pragma once
#include <d3dx12.h>
#include "LightTransform.h"
#include "..\\..\\ComPtr.h"

struct LightBuffer
{
	DirectX::XMFLOAT4 lightDirection;
	DirectX::XMFLOAT4 ambientColor;
	DirectX::XMFLOAT4 diffuseColor;
	DirectX::XMFLOAT4 cameraEyePos;    //カメラの位置
};

class DirectionalLight : public LightTransform
{
public:

	DirectX::XMMATRIX m_lightViewProj;
	DirectX::XMFLOAT3 m_lightPosition;	//この位置を中心に影を生成

	LightBuffer m_lightBuffer;

	float m_shadowDistance;
	float m_shadowScale;

	DirectionalLight();

	void Init(ID3D12Device* pDevice);

	void Update();

	ID3D12Resource* GetLightConstantBuffer() const;

private:
	ComPtr<ID3D12Resource> m_lightConstantBuffer;

	ID3D12Device* m_pDevice;
};