#pragma once

#include "System\\ComPtr.h"
#include "System\\Engine\\Engine.h"

#include "System\\Engine\\Core\\Color.h"

#include "KeyString.h"

#include "System\\Engine\\Core\\XMFLOATHelper.h"

//-----------------------------------------------------------------------------
// �����̃V�[���ŋ��p����f�[�^
//-----------------------------------------------------------------------------
namespace Common
{
	//-----------------------------------------------------------------------------
	// �V�[����ǉ������ۂɒ萔��ǉ����܂��B
	//-----------------------------------------------------------------------------
	enum {
		SCENE_INIT,
		SCENE_TITLE,
		SCENE_GAME,
		SCENE_CLEAR,
		SCENE_GAMEOVER,
		SCENE_EXIT,
	};

	//-----------------------------------------------------------------------------
	// �����̃V�[���ŋ��p����f�[�^���\���̂ɂ܂Ƃ߂܂��B
	//-----------------------------------------------------------------------------
	struct CommonData {
	};
}

namespace KeyString
{
	constexpr const char* FONT_NOTOSERIF = "Resource\\Fonts\\NotoSerifJP-Medium.ttf";

	constexpr const char* SOUND_INIT_BNK = "Resource\\Sounds\\Init.bnk";
	constexpr const char* SOUND_BGM_BNK = "Resource\\Sounds\\BGM.bnk";
	constexpr const char* SOUND_SE_BNK = "Resource\\Sounds\\SE.bnk";

	constexpr const char* SOUND_BGM_TITLE = "BGM_Title";

	constexpr const char* SOUND_SE_TITLE_NOISE = "Title_Noise";
	constexpr const char* SOUND_SE_TITLE_BLACKOUT = "Title_BlackOut";
}
