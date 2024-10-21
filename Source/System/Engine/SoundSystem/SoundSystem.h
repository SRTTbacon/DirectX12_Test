#pragma once
#include "bass.h"
#include "bassflac.h"
#include "bass_fx.h"
#include "string"
#include "unordered_map"

#include "..\\..\\Main\\Utility.h"

#undef PlaySound

//�T�E���h�Đ��V�X�e�� Bass Audio Library���g�p�B �c���ړI�̏ꍇ�͎g�p�s��
//�ڂ����� "https://www.un4seen.com/" ���Q��
//�Ή��t�H�[�}�b�g : Bass Audio Library���Ή����Ă������
//	.wav .mp3 .ogg .flac .aac .wma �ȂǂȂ� �v���O�C���𓱓�����΃v���X�őΉ���
//���p���p����ꍇ��Wwise�𐄏�


//�T�E���h�̏�Ԃ�ێ�����\����
struct SoundHandle
{
	double maxSoundTime;	//�T�E���h�̒��� (�b�P��)

	float defaultFrequency;	//�����̎��g��
	float speed;			//���x (0.0f�`)
	float volume;			//���� (0.0f�`1.0f)

	UINT streamHandle;		//�T�E���h�n���h��

	bool bPlaying;			//�Đ������ǂ���

	//�T�E���h�̍Đ�
	//���� : bool �ŏ�����Đ����邩 (false�̏ꍇ�APauseSound()���Ă񂾎��Ԃ���ĊJ)
	void PlaySound(bool bRestart = true);

	//�T�E���h�̈ꎞ��~
	void PauseSound();

	//�v���p�e�B(���x�≹�ʂȂ�) ���X�V
	void UpdateProperty() const;

	//�Đ��ʒu��ύX (���ΓI��)
	//���� : double ���݂̍Đ��ʒu����̑��Έʒu(�b)
	void ChangePosition(double relativeTime) const;

	//�Đ��ʒu��ύX
	//���� : double �ړ���̈ʒu (�b)
	void SetPosition(double toTime) const;

	//�T�E���h�����
	//���̊֐����s��͎g�p�s�ɂȂ�
	void Release();
};

class SoundSystem
{
public:
	//�R���X�g���N�^
	//���� : HWND �E�B���h�E�n���h��
	SoundSystem(HWND pHandle);

	~SoundSystem();

	//�t�@�C������T�E���h�̃n���h�����쐬
	//���� : std::string �T�E���h�̃t�@�C���p�X, bool �����ɍĐ����邩
	//�߂�l : �T�E���h�������Ă���n���h�� (HSTREAM = unsigned long)
	SoundHandle* LoadSound(std::string filePath, bool bPlay = false);

	//�Đ����̃T�E���h�����ׂčX�V
	void Update();

private:
	//�T�E���h���ꗗ
	std::vector<SoundHandle*> m_soundHandles;
};
