#pragma once

#include <EffekseerRendererDX12.h>

#include "..\\Camera\\Camera.h"
#include "..\\..\\Main\\Utility.h"
#include "Effect.h"

//Effekseer���g�p�����G�t�F�N�g�̕`��

class EffectManager
{
public:
	friend class Engine;

	EffectManager();
	~EffectManager();

	//�t�@�C������G�t�F�N�g�����[�h
	//@return �ǂݍ��񂾃G�t�F�N�g�𐧌䂷��N���X
	Effect* CreateEffect(std::string path);
	//����������G�t�F�N�g�����[�h
	Effect* CreateEffect(const void* data, int size);

private:
	Effekseer::ManagerRef m_effekseerManager;
	EffekseerRenderer::RendererRef m_effekseerRenderer;
	Effekseer::RefPtr<EffekseerRenderer::SingleFrameMemoryPool> m_efkMemoryPool;
	Effekseer::RefPtr<EffekseerRenderer::CommandList> m_efkCommandList;

	ID3D12GraphicsCommandList* m_pCommandList;
	Camera* m_pCamera;

	std::vector<std::unique_ptr<Effect>> m_effects;

	//������
	//@param ID3D12Device* : �G���W���̃f�o�C�X
	//@param ID3D12CommandQueue* : �G���W���̃R�}���h�L���[
	//@param ID3D12GraphicsCommandList* : �G���W���̃R�}���h���X�g
	//@param Camera* : �G���W���̃J�����N���X
	//@param UINT : �o�b�t�@�̐� (��{�I�ɂ̓G���W���̃o�b�t�@���Ɠ����l������)
	//@param int : �G�t�F�N�g�̍ő�`�搔 (�C���X�^���X�P�ʁB1�̃G�t�F�N�g�ɉ����C���X�^���X�����݂���ꍇ������)
	void Initialize(ID3D12Device* pDevice, ID3D12CommandQueue* pCommandQueue, ID3D12GraphicsCommandList* pCommandList, Camera* pCamera, UINT bufferCount, int maxEffectCount);

	//�G�t�F�N�g�̍X�V
	//�G���W�����Ŏ��s�����
	void LateUpdate();

	//�G�t�F�N�g�̃����_�����O�J�n
	//�G���W�����Ŏ��s�����
	void BeginRender();
	//�G�t�F�N�g�̃����_�����O�I��
	//�G���W�����Ŏ��s�����
	void EndRender();

	//�G�t�F�N�g��`��
	//�G���W�����Ŏ��s�����
	void Draw();

	//XMMATRIX��Effekseer�Ŏg�p����Matrix44�ɕϊ�
	Effekseer::Matrix44 ConvertMatrix(DirectX::XMMATRIX& mat);

	Effect* AddEffect(Effekseer::EffectRef effect);
};
