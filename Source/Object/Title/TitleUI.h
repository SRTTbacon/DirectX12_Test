#pragma once

#include "..\\..\\GameBase.h"
#include "..\\..\\Scene\\Scene\\Scene.h"
#include "..\\FileSystem\\HCSFileSystem.h"
#include "..\\GUI\\GUILayout.h"
#include "..\\Game\\Opening\\TextSystem\\SpecialTextAnimation.h"

class TitleUI
{
public:
	TitleUI(SceneData* pSceneData);
	~TitleUI();

	void Initilaize();

	void Update();

	void Draw();

public:
	inline bool GetIsStarting() const { return m_gameStarting; }

private:
	static const char* TITLE_TEXTURE_PATH;
	static const char* BUTTON_EXIT_01;
	static const char* BUTTON_EXIT_02;
	static const char* TEXTURE_WWISE_ICON;
	static const char* BLACKOUT_HCS_PATH;
	static const char* BLACKOUT_PATH;
	static const float OPENING_TIME;
	static const float FRIEZE_TIME;
	static const float FRIEZE_NOISE_START_TIME;
	static const float BLACKOUT_FRAME_TIME;
	static const float BLACKOUT_EXIT_TIME;

	std::vector<Texture2D*> m_blackOuts;

	SceneData* m_pSceneData;

	GUIButton m_exitButton;
	GUIAnimationText m_uiTextEngine;
	GUIAnimationTexture m_uiTextureEngine;

	SpecialTextAnimation m_openingTextAnimation;

	UITextureRef m_uiBlackBack;
	UITextureRef m_uiBlackOutTexture;

	float m_sceneTime;
	float m_friezeTime;
	float m_frameTimeBlackOut;
	float m_frameTimeBlackOutExit;
	int m_nowBlackOutFrame;

	bool m_gameStarting;
	bool m_bCanOperation;
	bool m_bStartedOpening;

	void UpdateBlackBack();
	void UpdateFrieze();
	void UpdataBlackOut();
};
