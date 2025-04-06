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
		BlinkNone,		//瞬きを行っていない
		BlinkStart,		//目を閉じている途中
		BlinkEnd		//目を開く途中
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
	//棒立ちキャラクターの瞬き
	void UpdateBlink();

	//棒立ちキャラクターの次の瞬きまでの時間を指定
	void SetNextBlinkTime();
};
