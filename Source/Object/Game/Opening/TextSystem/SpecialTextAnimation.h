#pragma once

#include "..\\..\\..\\..\\GameBase.h"

//画面中央に配置するアニメーションテキスト
//黒背景と白背景を交互に出し、文字もそれに合わせて色を変える

class SpecialTextAnimation
{
public:
	SpecialTextAnimation();

	void Initialize(std::string strFontPath, UINT fontSize, std::string text);

	void SetNextAnimation(std::string strFontPath, UINT fontSize, std::string text);

	void StartAnimation();

	void StopAnimation();

	void Update();

	void Draw();

	inline std::shared_ptr<SpecialTextAnimation> GetNextAnimation() const { return m_pNextTextAnimation; }
	inline bool GetIsPlaying() const { return m_bAnimation || m_animationTime >= 0.0f; }

public:
	UITextRef m_text;

	DXGI_RGBA m_firstBackColor;
	DXGI_RGBA m_firstTextColor;
	DXGI_RGBA m_secondBackColor;
	DXGI_RGBA m_secondTextColor;
	DXGI_RGBA m_endBackColor;
	DXGI_RGBA m_endTextColor;

	float m_startIntervalTime;
	float m_firstIntervalTime;
	float m_secondIntervalTime;
	float m_endTime;

	bool m_bEnableSecondColor;

private:
	std::shared_ptr<SpecialTextAnimation> m_pNextTextAnimation;

	UITextureRef m_whiteTexture;

	float m_animationTime;

	bool m_bAnimation;

	void ResetToNextAnimation();
};
