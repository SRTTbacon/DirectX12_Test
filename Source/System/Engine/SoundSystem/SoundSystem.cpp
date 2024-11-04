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
		if (m_soundHandles[i] && m_soundHandles[i]->streamHandle > 0) {
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
	pHandle->maxSoundTime = BASS_ChannelBytes2Seconds(fxHandle, BASS_ChannelGetLength(fxHandle, BASS_POS_BYTE));
	pHandle->bPlaying = bPlay;

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
		if (pSound->streamHandle > 0) {
			//LoadSound��BASS_CONFIG_BUFFER��100�ɐݒ肷��֌W�ŁAPC�ɂ���Ă͉����u�c�u�c����̂�h��
			BASS_ChannelUpdate(pSound->streamHandle, 300);
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

SoundHandle::SoundHandle(const UINT handle, const float freq)
	: streamHandle(handle)
	, defaultFrequency(freq)
	, speed(1.0f)
	, volume(1.0f)
	, maxSoundTime(0.0)
	, bPlaying(false)
{
}

//�T�E���h���Đ�
//���� : �ŏ�����Đ����邩 (false�̏ꍇ�APauseSound()���Ă񂾎��Ԃ���ĊJ)
void SoundHandle::PlaySound(bool bRestart)
{
	if (streamHandle == 0)
		return;

	UpdateProperty();

	BASS_ChannelPlay(streamHandle, bRestart);

	if (GetBassError) {
		printf("�T�E���h�̍Đ����ɃG���[���������܂����B�G���[�R�[�h:%d\n", BASS_ErrorGetCode());
		return;
	}

	bPlaying = true;
}

//�T�E���h�̈ꎞ��~
void SoundHandle::PauseSound()
{
	if (streamHandle == 0)
		return;

	BASS_ChannelPause(streamHandle);

	bPlaying = false;
}

//�v���p�e�B(���x�≹�ʂȂ�)���X�V
void SoundHandle::UpdateProperty() const
{
	//�n���h�����Ȃ�������A�Đ�����~���Ă���ꍇ�͏������I���� (�Đ��J�n���ɌĂ΂�邽��)
	if (streamHandle == 0 || !bPlaying)
		return;

	BASS_ChannelSetAttribute(streamHandle, BASS_ATTRIB_TEMPO_FREQ, defaultFrequency * speed);
	BASS_ChannelSetAttribute(streamHandle, BASS_ATTRIB_VOL, volume);
}

//�Đ��ʒu��ύX
//���� : float ���݂̍Đ��ʒu����̑��Έʒu(�b)
void SoundHandle::ChangePosition(double relativeTime) const
{
	if (streamHandle == 0)
		return;

	//���݂̍Đ����Ԃ��擾
	QWORD position = BASS_ChannelGetPosition(streamHandle, BASS_POS_BYTE);
	double nowPosition = BASS_ChannelBytes2Seconds(streamHandle, position);

	//���Ύ��Ԃ�ǉ�
	nowPosition += relativeTime;

	//�Đ��ʒu��ύX
	SetPosition(nowPosition);
}

//�Đ��ʒu��ύX
//���� : double �ړ���̈ʒu (�b)
void SoundHandle::SetPosition(double toTime) const
{
	if (streamHandle == 0)
		return;

	//�͈͐���
	if (toTime < 0.0)
		toTime = 0.0f;
	else if (toTime > maxSoundTime)
		toTime = maxSoundTime;

	//�Đ��ʒu��toTime�ɕύX (�b�ł̎w��͂ł��Ȃ����߁A�b����BASS_POS_BYTE�֕ϊ�)
	BASS_ChannelSetPosition(streamHandle, BASS_ChannelSeconds2Bytes(streamHandle, toTime), BASS_POS_BYTE);
}

//�T�E���h�̉��
void SoundHandle::Release() const
{
	if (streamHandle == 0)
		return;

	BASS_ChannelStop(streamHandle);
	BASS_StreamFree(streamHandle);
}
