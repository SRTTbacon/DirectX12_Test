#include "SoundSystem.h"

#undef PlaySound

#define GetBassError BASS_ErrorGetCode() != BASS_OK

//�R���X�g���N�^
//���� : HWND �E�B���h�E�n���h��
SoundSystem::SoundSystem(HWND pHandle)
{
	//����1 : �Đ��f�o�C�X (-1�����ݑI������Ă���f�o�C�X�ŁA�C���f�b�N�X��ύX����ƁA�Ⴆ�΃��j�^�[�̃X�s�[�J�[�Ȃǂ���Đ������B��{-1)
	//����2 : �T���v�����O���[�g (44.1kHz�A48kHz�A96kHz�A192kHz�Ȃǂ��嗬�B���l���傫���Ȃ�قǃT�E���h����ł͍������ɂȂ邪�A���ׂ��傫���Ȃ�)
	//����3 : �t���O (�X�e���I�Đ��ł���Ί�{0�w��B3D�\�����s���Ƃ���A�����ă��m�����ōĐ�����̂ł���� "BASS_DEVICE_*" �Ŏw��)
	//����4 : �悭�킩��񂩂�nullptr
	BASS_Init(-1, 96000, 0, pHandle, nullptr);
}

SoundSystem::~SoundSystem()
{
	//���ׂẴ��[�h����Ă���T�E���h�����
	for (UINT i = 0; i < m_soundHandles.size(); i++) {
		if (m_soundHandles[i] && m_soundHandles[i]->m_streamHandle > 0) {
			m_soundHandles[i]->Release();

			delete m_soundHandles[i];
		}
	}
}

//�t�@�C������T�E���h�̃n���h�����쐬
//�߂�l : �T�E���h��񂪓������\����
SoundHandle* SoundSystem::LoadSound(std::string filePath, bool bPlay)
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

	//�s�b�`��G�t�F�N�g���K���ł���`�ɕύX
	int fxHandle = BASS_FX_TempoCreate(soundHandle, BASS_FX_FREESOURCE);

	if (GetBassError) {
		printf("�T�E���h�̓ǂݍ��ݎ��ɃG���[���������܂����B�G���[�R�[�h:%d\n", BASS_ErrorGetCode());
		BASS_StreamFree(soundHandle);
		return nullptr;
	}

	//�T�E���h�̏�Ԃ�ێ�����\����
	float freq;
	BASS_ChannelGetAttribute(fxHandle, BASS_ATTRIB_TEMPO_FREQ, &freq);
	SoundHandle* pHandle = new SoundHandle(fxHandle, freq);

	//�Đ��I�����̃R�[���o�b�N��ݒ�
	HSYNC syncHandle = BASS_ChannelSetSync(fxHandle, BASS_SYNC_END, 0, &SoundSystem::PlaybackEndCallback, pHandle);
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

void SoundSystem::Update()
{
	for (UINT i = 0; i < m_soundHandles.size(); i++) {
		SoundHandle* pSound = m_soundHandles[i];
		if (pSound->m_streamHandle > 0) {
			//LoadSound��BASS_CONFIG_BUFFER��100�ɐݒ肷��֌W�ŁAPC�ɂ���Ă͉����u�c�u�c����̂�h��
			BASS_ChannelUpdate(pSound->m_streamHandle, 300);
		}
		else {
			//�T�E���h��Release����Ă����烁��������폜
			delete pSound;
			std::vector<SoundHandle*>::iterator it = m_soundHandles.begin();
			it += i;
			m_soundHandles.erase(it);
		}
	}
}

void SoundSystem::PlaybackEndCallback(HSYNC handle, DWORD channel, DWORD data, void* user)
{
	SoundHandle* pSoundHandle = static_cast<SoundHandle*>(user);
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

SoundHandle::SoundHandle(const UINT handle, const float freq)
	: m_streamHandle(handle)		//�n���h��
	, m_maxSoundTime(BASS_ChannelBytes2Seconds(handle, BASS_ChannelGetLength(handle, BASS_POS_BYTE)))	//�T�E���h�̒���
	, m_defaultFrequency(freq)	//�����̎��g��
	, m_speed(1.0f)				//�Đ����x
	, m_volume(1.0f)				//����
	, m_bLooping(false)
	, m_bPlaying(false)
{
}

SoundHandle::~SoundHandle()
{
	Release();
}

//�T�E���h���Đ�
//���� : �ŏ�����Đ����邩 (false�̏ꍇ�APauseSound()���Ă񂾎��Ԃ���ĊJ)
void SoundHandle::PlaySound(bool bRestart)
{
	if (m_streamHandle == 0)
		return;

	BASS_ChannelPlay(m_streamHandle, bRestart);

	if (GetBassError) {
		printf("�T�E���h�̍Đ����ɃG���[���������܂����B�G���[�R�[�h:%d\n", BASS_ErrorGetCode());
		m_bPlaying = false;
		return;
	}

}

//�T�E���h�̈ꎞ��~
void SoundHandle::PauseSound()
{
	if (m_streamHandle == 0)
		return;

	BASS_ChannelPause(m_streamHandle);

	m_bPlaying = false;
}

//�v���p�e�B(���x�≹�ʂȂ�)���X�V
void SoundHandle::UpdateProperty() const
{
	//�n���h�����Ȃ�������A�Đ�����~���Ă���ꍇ�͏������I���� (�Đ��J�n���ɌĂ΂�邽��)
	if (m_streamHandle == 0)
		return;

	//�Đ����x�Ɖ��ʂ�ύX
	BASS_ChannelSetAttribute(m_streamHandle, BASS_ATTRIB_TEMPO_FREQ, m_defaultFrequency * m_speed);
	BASS_ChannelSetAttribute(m_streamHandle, BASS_ATTRIB_VOL, m_volume);
}

//�Đ��ʒu��ύX
//���� : float ���݂̍Đ��ʒu����̑��Έʒu(�b)
void SoundHandle::ChangePosition(double relativeTime) const
{
	if (m_streamHandle == 0)
		return;

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
void SoundHandle::SetPosition(double toTime) const
{
	if (m_streamHandle == 0)
		return;

	//�͈͐���
	if (toTime < 0.0)
		toTime = 0.0f;
	else if (toTime > m_maxSoundTime)
		toTime = m_maxSoundTime;

	//�Đ��ʒu��toTime�ɕύX (�b�ł̎w��͂ł��Ȃ����߁A�b����BASS_POS_BYTE�֕ϊ�)
	BASS_ChannelSetPosition(m_streamHandle, BASS_ChannelSeconds2Bytes(m_streamHandle, toTime), BASS_POS_BYTE);
}

//�T�E���h�̉��
void SoundHandle::Release() const
{
	BASS_CHANNELINFO info;

	//���ɃT�E���h��������Ă���Ώ������I����
	if (m_streamHandle == 0 || BASS_ChannelGetInfo(m_streamHandle, &info))
		return;

	BASS_ChannelStop(m_streamHandle);
	BASS_StreamFree(m_streamHandle);
}
