#pragma once
#include <d3dx12.h>
#include "..\\Model\\Transform\\Transform.h"
#include "..\\..\\ComPtr.h"

struct LightBuffer
{
	DirectX::XMFLOAT4 lightDirection;	//ライトの方向
	DirectX::XMFLOAT4 ambientColor;		//影の色
	DirectX::XMFLOAT4 diffuseColor;		//ライトの色
	DirectX::XMFLOAT4 cameraEyePos;		//カメラの位置
	DirectX::XMFLOAT4 fogColor;			//フォグの色
	DirectX::XMFLOAT2 fogStartEnd;		//フォグの開始距離と終了距離
};

class DirectionalLight : public Transform
{
public:

	DirectX::XMMATRIX m_lightViewProj;	//ディレクショナルライトのビューマトリクス
	DirectX::XMFLOAT3 m_lightPosition;	//この位置を中心に影を生成

	LightBuffer m_lightBuffer;			//各ピクセルシェーダーに送信するライト情報

	float m_shadowDistance;
	float m_shadowScale;

	DirectionalLight();

	void Init(ID3D12Device* pDevice);

	ComPtr<ID3D12Resource> CreateConstantBuffer();

	void LateUpdate();
	void MemCopyBuffer(void* p) const;

private:
	//影の位置を更新する間隔 (常に更新するとセルフシャドウがちらつくため)
	static const float SHADOW_GRID_SIZE;

	ID3D12Device* m_pDevice;
};