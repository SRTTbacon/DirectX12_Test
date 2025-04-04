#pragma once
#include <Effekseer.h>
#include <DirectXMath.h>

#include "..\\Core\\XMFLOATHelper.h"

class EffectHandle
{
public:
	friend class Effect;

	EffectHandle(const Effekseer::ManagerRef effekseerManager, const Effekseer::Handle handle, const DirectX::XMFLOAT3* pParentScale);

	//�ĊJ
	void Play();
	//�ꎞ��~
	void Pause();
	//�G�t�F�N�g���~���j������B���̊֐��ȍ~��EffectHandle�N���X�͎g�p�s��
	void Stop();

	//�Đ����x��ύX (0.0�`1.0f�`n)
	void SetSpeed(float speed) const;

	void SetScale(float scale);

public:
	//���݂̍Đ����x���擾
	float GetSpeed() const;

	//���[���h�X�P�[�����擾
	DirectX::XMFLOAT3 GetWorldScale() const;

	//�G�t�F�N�g�����݂��Ă��邩
	inline bool GetIsAlive() const { return m_bAlive; }

public:
	//�G�t�F�N�g�̈ʒu
	DirectX::XMFLOAT3 m_position;
	//���̃G�t�F�N�g�̃��[�J���X�P�[��
	DirectX::XMFLOAT3 m_localScale;
	bool m_bDraw;

private:
	const Effekseer::ManagerRef m_effekseerManager;
	const Effekseer::Handle m_handle;

	//�G�t�F�N�g�S�̂̃X�P�[��
	const DirectX::XMFLOAT3* m_pParentScale;

	bool m_bAlive;		//�g�p�\�ȃG�t�F�N�g��
	bool m_bPlaying;	//�Đ�����

	bool GetIsExistEffect() const;

	//�ʒu��X�P�[�����X�V
	//Effect�N���X������s�����
	void Update();
};

class Effect
{
public:
	Effect(Effekseer::ManagerRef effekseerManager, Effekseer::EffectRef effect, DirectX::XMVECTOR* pCameraPos);

	//�G�t�F�N�g���Đ�
	//@param XMFLOAT3 �����ʒu
	//@param bool �����G�t�F�N�g���폜���Ă���Đ�
	//@return �Đ����̃G�t�F�N�g�̃n���h��
	EffectHandle* Play(DirectX::XMFLOAT3 position, bool bOnceFalse = false);

	//�G�t�F�N�g�S�̂̃X�P�[����ݒ�
	//@param float �X�P�[�� (x, y, z�ɓ����l������)
	void SetScale(float scale);
	//�G�t�F�N�g�S�̂̃X�P�[����ݒ�
	//@param XMFLOAT3 �X�P�[��
	void SetScale(DirectX::XMFLOAT3 scale);

	//�`�拗����ݒ�
	//�J�����Ƃ̋����͂��̒l�ȏ�ɂȂ�ƕ`�悳��Ȃ��Ȃ�
	//param float ����
	void SetHiddenDistance(float distancePow);

	//�Đ����I�������n���h�������
	void Update();

	void Release();

public:
	//�`�拗�����擾
	inline float GetHiddenDistance() const { return m_hiddenDistance; }

	inline bool GetIsReleased() const { return m_bReleased; }

private:
	Effekseer::ManagerRef m_effekseerManager;
	Effekseer::EffectRef m_effect;

	std::vector<std::unique_ptr<EffectHandle>> m_handles;	//���̃^�C�v�̂��ׂẴG�t�F�N�g���Ǘ�

	DirectX::XMVECTOR* m_pCameraPos;
	DirectX::XMFLOAT3 m_scale;

	//�\������
	float m_hiddenDistance;

	bool m_bReleased;
};
