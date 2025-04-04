#pragma once

#include <Windows.h>
#include <DirectXMath.h>

#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkMemoryMgrModule.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#include "AK/AkDefaultIOHookDeferred.h"
#include <random>
#include <fstream>
#include <unordered_map>

//3D������������T�E���h�G���W��"Wwise"���g�p���邽�߂̃��C�u����
//���_�Ƃ�Detroit�Ƃ�Overwatch2�Ƃ��Ŏg�p����Ă��邽�߁A�M������
//�{���Ȃ�T�E���h�G���W�����ƃv���W�F�N�g�ɓ����̂���ʓI�����ǁA�����dll���g�p

struct Emitter
{
	const AkGameObjectID objectID;

	DirectX::XMFLOAT3 position;	//�Đ��ʒu
	DirectX::XMFLOAT3 front;		//�O�����x�N�g��
	DirectX::XMFLOAT3 top;		//������x�N�g��

	Emitter(AkGameObjectID id);
};

using EmitterRef = std::shared_ptr<Emitter>;

class WwiseSoundHandle
{
public:
	WwiseSoundHandle(const EmitterRef pEmitter, AkPlayingID playingID);

public:
	inline EmitterRef GetEmitter() const { return m_pEmitter; }
	inline AkPlayingID GetPlayingID() const { return m_playingID; }

private:
	const EmitterRef m_pEmitter;
	const AkPlayingID m_playingID;
};

class WwiseSoundSystem
{
public:
	//Wwise�̏�����
	//���� : char* init.bnk�̃t�@�C���p�X
	UINT Initialize(const char* initBNKStr);

	//�X�V (�����Đ����͖��t���[����1�x���s������)
	void Update();

	//�G�~�b�^�[��ǉ�
	//�ʒu���ω�����I�u�W�F�N�g�̐������ǉ�����
	EmitterRef Add_Emitter();

	//�G�~�b�^�[���폜
	//���� : Emitter Add_Emitter()�Ŏ擾
	void Delete_Emitter(const EmitterRef emitter);

	//.bnk�t�@�C�������[�h
	//���� : char* .bnk�t�@�C���̃p�X
	//�߂�l : �G���[�R�[�h
	AKRESULT Load_Bank(const char* bnkFileStr);

	//.bnk���A�����[�h
	//���� : char* .bnk�t�@�C���̃p�X
	//�߂�l : �G���[�R�[�h
	AKRESULT UnLoad_Bank(const char* bnkFileStr);

	//�C�x���g�����Đ�
	//���� : char* �C�x���g��
	//�߂�l : �T�E���h�n���h��
	WwiseSoundHandle Play(const char* eventNameStr);

	//3D��̈ʒu���ω�����C�x���g���Đ�
	//���� : char* �C�x���g��, Emitter �G�~�b�^�[
	//�߂�l : �T�E���h�n���h��
	WwiseSoundHandle Play(const char* eventNameStr, const EmitterRef emitter);

	//�C�x���gID���Đ�
	//���� : UINT �C�x���gID
	//�߂�l : �T�E���h�n���h��
	WwiseSoundHandle Play(UINT eventID);

	//3D��̈ʒu���ω�����C�x���g���Đ�
	//���� : UINT �C�x���gID, Emitter �G�~�b�^�[
	//�߂�l : �T�E���h�n���h��
	WwiseSoundHandle Play(UINT eventID, const EmitterRef emitter);

	//�C�x���g���~
	//���� : UINT Play()�Ŏ擾����ID
	void Stop(WwiseSoundHandle soundHandle);

	//���ׂẴC�x���g���~
	void Stop_All();

	//���ׂẴC�x���g���ꎞ��~
	//�߂�l : ����������true
	bool Pause_All();

	//�ꎞ��~���̂��ׂẴC�x���g���Đ�(�r������)
	//�߂�l : ����������true
	bool Play_All();

	//State��ݒ�
	//���� : char* �e�X�e�[�g�̖��O, char* �q�X�e�[�g�̖��O
	//�߂�l : ����������true
	bool Set_State(const char* parentNameStr, const char* childNameStr);

	//RTPC��ݒ�
	//���� : char* RTPC�̖��O, float ���l(Wwise���Őݒ肵���͈�)
	//�߂�l : ����������true
	bool Set_RTPC(const char* rtpcNameStr, float value);

	//���X�i�[�̈ʒu�A�p�x��ݒ�
	//����(�ȗ�) : 3D���xyz�̈ʒu, �O����(���K��), �����(���K��)
	void Set_Listener_Position(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& front, const DirectX::XMFLOAT3& top);

	//�G�~�b�^�[�̈ʒu�A�p�x���X�V
	//���� : Emitter �G�~�b�^�[
	void Set_Emitter(const EmitterRef emitter);

	//Wwise�����
	//���̊֐������s��͍ēxInit()�����s����܂Ŏg�p�ł��܂���B
	void Dispose();

	//����������Ă��邩
	bool IsInited();

	//Wwise���Ō�ɏo�͂����G���[���擾
	UINT Get_Result_Index() const;

private:
	CAkDefaultIOHookDeferred m_lowLevelIO;

	AKRESULT m_lastResult;
	UINT m_initBankID;

	AkGameObjectID m_listenerID = 0;

	AkGameObjectID GetRandomGameObjectID();

	std::unordered_map<std::string, char*> m_pBankData;

	static void AkCallbackFunction(AkCallbackType in_eType, AkCallbackInfo* in_pCallbackInfo);
};
