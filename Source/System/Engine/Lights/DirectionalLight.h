#pragma once
#include <d3dx12.h>
#include "..\\Model\\Transform\\Transform.h"
#include "..\\..\\ComPtr.h"

struct LightBuffer
{
	DirectX::XMFLOAT4 lightDirection;	//���C�g�̕���
	DirectX::XMFLOAT4 ambientColor;		//�e�̐F
	DirectX::XMFLOAT4 diffuseColor;		//���C�g�̐F
	DirectX::XMFLOAT4 cameraEyePos;		//�J�����̈ʒu
	DirectX::XMFLOAT4 fogColor;			//�t�H�O�̐F
	DirectX::XMFLOAT2 fogStartEnd;		//�t�H�O�̊J�n�����ƏI������
};

class DirectionalLight : public Transform
{
public:

	DirectX::XMMATRIX m_lightViewProj;	//�f�B���N�V���i�����C�g�̃r���[�}�g���N�X
	DirectX::XMFLOAT3 m_lightPosition;	//���̈ʒu�𒆐S�ɉe�𐶐�

	LightBuffer m_lightBuffer;			//�e�s�N�Z���V�F�[�_�[�ɑ��M���郉�C�g���

	float m_shadowDistance;
	float m_shadowScale;

	DirectionalLight();

	void Init(ID3D12Device* pDevice);

	ComPtr<ID3D12Resource> CreateConstantBuffer();

	void LateUpdate();
	void MemCopyBuffer(void* p) const;

private:
	//�e�̈ʒu���X�V����Ԋu (��ɍX�V����ƃZ���t�V���h�E�����������)
	static const float SHADOW_GRID_SIZE;

	ID3D12Device* m_pDevice;
};