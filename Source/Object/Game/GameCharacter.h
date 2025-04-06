#pragma once

#include "..\\..\\GameBase.h"

class GameCharacter
{
public:
	GameCharacter();

	void Initialize();

	void Update();

private:
	enum BlinkMode
	{
		BlinkNone,		//�u�����s���Ă��Ȃ�
		BlinkStart,		//�ڂ���Ă���r��
		BlinkEnd		//�ڂ��J���r��
	};

	static const std::string CHARACTER_FILE;

	Character* m_pPoseChar;
	Character* m_pDanceChar1;
	Character* m_pDanceChar2;

	BassSoundHandle* m_pBGMHandle;

	BlinkMode m_blinkMode;

	float m_animSpeed;
	float m_nowBlinkTime;
	float m_nextBlinkTime;

	bool m_bAnim;

	void UpdateKeys();
	//�_�����L�����N�^�[�̏u��
	void UpdateBlink();

	//�_�����L�����N�^�[�̎��̏u���܂ł̎��Ԃ��w��
	void SetNextBlinkTime();
};
