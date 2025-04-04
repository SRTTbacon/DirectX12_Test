#include "BassSoundSystem.h"

#undef PlaySound

#define GetBassError BASS_ErrorGetCode() != BASS_OK

//�R���X�g���N�^
//���� : HWND �E�B���h�E�n���h��
BassSoundSystem::BassSoundSystem(HWND pHandle)
{
	//����1 : �Đ��f�o�C�X (-1������Windows���I�����Ă���f�o�C�X�ŁA�C���f�b�N�X��ύX����ƁA�Ⴆ�΃��j�^�[�̃X�s�[�J�[�Ȃǂ���Đ������B��{-1)
	//����2 : �T���v�����O���[�g (44.1kHz�A48kHz�A96kHz�A192kHz�Ȃǂ��嗬�B���l���傫���Ȃ�قǃT�E���h����ł͍������ɂȂ邪�A���ׂ��傫���Ȃ�)
	//����3 : �t���O (�X�e���I�Đ��ł���Ί�{0�w��B3D�\�����s���Ƃ���A�����ă��m�����ōĐ�����̂ł���� "BASS_DEVICE_* (enum�^)" �Ŏw��)
	//����4 : �悭�킩��񂩂�nullptr
	BASS_Init(-1, 96000, 0, pHandle, nullptr);
}

BassSoundSystem::~BassSoundSystem()
{
	//���ׂẴ��[�h����Ă���T�E���h�����
	/*for (SoundHandle* pHandle : m_soundHandles) {
		if (pHandle) {
			pHandle->Release();

			delete pHandle;
		}
	}*/
	//m_soundHandles.clear();
	BASS_Free();
}

//�t�@�C������T�E���h�̃n���h�����쐬
//�߂�l : �T�E���h��񂪓������N���X
BassSoundHandle* BassSoundSystem::LoadSound(std::string filePath, bool bPlay)
{
	UINT soundHandle = 0;

	BASS_SetConfig(BASS_CONFIG_BUFFER, 100);

	//.flac�`���͏��X����Ȃ��ߊ֐���������Ă���
	if (GetFileExtension(filePath) == ".flac") {
		soundHandle = BASS_FLAC_StreamCreateFile(false, filePath.c_str(), 0, 0, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE);
	}
	else {
		soundHandle = BASS_StreamCreateFile(false, filePath.c_str(), 0, 0, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE);
	}

	if (GetBassError) {
		printf("�T�E���h�̓ǂݍ��ݎ��ɃG���[���������܂����B�G���[�R�[�h:%d\n", BASS_ErrorGetCode());
		return nullptr;
	}

	BassSoundHandle* pHandle = LoadSoundFromHandle(soundHandle, bPlay);

	return pHandle;
}

BassSoundHandle* BassSoundSystem::LoadSound(const void* mem, size_t size, bool bFlacFormat, bool bPlay)
{
	UINT soundHandle = 0;

	BASS_SetConfig(BASS_CONFIG_BUFFER, 100);

	//.flac�`���͏��X����Ȃ��ߊ֐���������Ă���
	if (bFlacFormat) {
		soundHandle = BASS_FLAC_StreamCreateFile(true, mem, 0, size, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE);
	}
	else {
		soundHandle = BASS_StreamCreateFile(true, mem, 0, size, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE);
	}

	if (GetBassError) {
		printf("�T�E���h�̓ǂݍ��ݎ��ɃG���[���������܂����B�G���[�R�[�h:%d\n", BASS_ErrorGetCode());
		return nullptr;
	}

	BassSoundHandle* pHandle = LoadSoundFromHandle(soundHandle, bPlay);

	return pHandle;
}

void BassSoundSystem::Update()
{
	for (UINT i = 0; i < m_soundHandles.size(); i++) {
		BassSoundHandle* pSound = m_soundHandles[i];
		if (pSound && pSound->m_streamHandle > 0) {
			//LoadSound��BASS_CONFIG_BUFFER��100�ɐݒ肷��֌W�ŁAPC�ɂ���Ă͉����u�c�u�c����̂�h��
			BASS_ChannelUpdate(pSound->m_streamHandle, 300);
		}
		else {
			//�T�E���h��Release����Ă����烁��������폜
			if (pSound) {
				delete pSound;
			}
			std::vector<BassSoundHandle*>::iterator it = m_soundHandles.begin();
			it += i;
			m_soundHandles.erase(it);
		}
	}
}

void BassSoundSystem::PlaybackEndCallback(HSYNC handle, DWORD channel, DWORD data, void* user)
{
	BassSoundHandle* pSoundHandle = static_cast<BassSoundHandle*>(user);
	//���̃T�E���h�����[�v�Đ��������Ă���΁A�ēx�Đ�
	if (pSoundHandle && pSoundHandle->m_bLooping) {
		if (pSoundHandle->m_bLooping) {
			pSoundHandle->PlaySound();
		}
		else {
			pSoundHandle->PauseSound();
		}
	}
}

BassSoundHandle* BassSoundSystem::LoadSoundFromHandle(int handleID, bool bPlay)
{
	//�s�b�`��G�t�F�N�g���K���ł���`�ɕύX
	int fxHandle = BASS_FX_TempoCreate(handleID, BASS_FX_FREESOURCE);

	if (GetBassError) {
		printf("�T�E���h�̓ǂݍ��ݎ��ɃG���[���������܂����B�G���[�R�[�h:%d\n", BASS_ErrorGetCode());
		BASS_StreamFree(handleID);
		return nullptr;
	}

	//�T�E���h�̏�Ԃ�ێ�����\����
	float freq;
	BASS_ChannelGetAttribute(fxHandle, BASS_ATTRIB_TEMPO_FREQ, &freq);
	BassSoundHandle* pHandle = new BassSoundHandle(fxHandle, freq);

	//�Đ��I�����̃R�[���o�b�N��ݒ�
	HSYNC syncHandle = BASS_ChannelSetSync(fxHandle, BASS_SYNC_END, 0, &BassSoundSystem::PlaybackEndCallback, pHandle);
	if (!syncHandle) {
		return nullptr;
	}

	//���[�h�シ���ɍĐ�
	if (bPlay) {
		pHandle->PlaySound(true);
	}

	m_soundHandles.push_back(pHandle);
	return pHandle;
}

BassSoundHandle::BassSoundHandle(const UINT handle, const float freq)
	: m_streamHandle(handle)	//�n���h��
	, m_maxSoundTime(BASS_ChannelBytes2Seconds(handle, BASS_ChannelGetLength(handle, BASS_POS_BYTE)))	//�T�E���h�̒���
	, m_defaultFrequency(freq)	//�����̎��g��
	, m_speed(1.0f)				//�Đ����x
	, m_volume(1.0f)			//����
	, m_bLooping(false)
	, m_bPlaying(false)
{
}

BassSoundHandle::~BassSoundHandle()
{
	Release();
}

//�T�E���h���Đ�
//���� : �ŏ�����Đ����邩 (false�̏ꍇ�APauseSound()���Ă񂾎��Ԃ���ĊJ)
void BassSoundHandle::PlaySound(bool bRestart)
{
	if (!IsVaild()) {
		return;
	}

	BASS_ChannelPlay(m_streamHandle, bRestart);

	if (GetBassError) {
		printf("�T�E���h�̍Đ����ɃG���[���������܂����B�G���[�R�[�h:%d\n", BASS_ErrorGetCode());
		m_bPlaying = false;
		return;
	}

}

//�T�E���h�̈ꎞ��~
void BassSoundHandle::PauseSound()
{
	if (!IsVaild()) {
		return;
	}

	BASS_ChannelPause(m_streamHandle);

	m_bPlaying = false;
}

//�v���p�e�B(���x�≹�ʂȂ�)���X�V
void BassSoundHandle::UpdateProperty()
{
	if (!IsVaild()) {
		return;
	}

	m_speed = max(m_speed, 0.0f);

	m_volume = min(m_volume, 1.0f);
	m_volume = max(m_volume, 0.0f);

	//�Đ����x�Ɖ��ʂ�ύX
	BASS_ChannelSetAttribute(m_streamHandle, BASS_ATTRIB_TEMPO_FREQ, m_defaultFrequency * m_speed);
	BASS_ChannelSetAttribute(m_streamHandle, BASS_ATTRIB_VOL, m_volume);
}

//�Đ��ʒu��ύX
//���� : float ���݂̍Đ��ʒu����̑��Έʒu(�b)
void BassSoundHandle::ChangePosition(double relativeTime) const
{
	if (!IsVaild()) {
		return;
	}

	//���݂̍Đ����Ԃ��擾
	QWORD position = BASS_ChannelGetPosition(m_streamHandle, BASS_POS_BYTE);
	double nowPosition = BASS_ChannelBytes2Seconds(m_streamHandle, position);

	//���Ύ��Ԃ�ǉ�
	nowPosition += relativeTime;

	//�Đ��ʒu��ύX
	SetPosition(nowPosition);
}

//�Đ��ʒu��ύX
//���� : double �ړ���̈ʒu (�b)
void BassSoundHandle::SetPosition(double toTime) const
{
	if (!IsVaild()) {
		return;
	}

	//�͈͐���
	if (toTime < 0.0)
		toTime = 0.0f;
	else if (toTime > m_maxSoundTime)
		toTime = m_maxSoundTime;

	//�Đ��ʒu��toTime�ɕύX (�b�ł̎w��͂ł��Ȃ����߁A�b����BASS_POS_BYTE�֕ϊ�)
	BASS_ChannelSetPosition(m_streamHandle, BASS_ChannelSeconds2Bytes(m_streamHandle, toTime), BASS_POS_BYTE);
}

//�T�E���h�̉��
void BassSoundHandle::Release() const
{
	if (!IsVaild()) {
		return;
	}

	BASS_ChannelStop(m_streamHandle);
	BASS_StreamFree(m_streamHandle);
}

bool BassSoundHandle::IsVaild() const
{
	BASS_CHANNELINFO info;

	//���ɃT�E���h��������Ă���Ώ������I����
	if (m_streamHandle == 0 || !BASS_ChannelGetInfo(m_streamHandle, &info)) {
		return false;
	}

	return true;
}
