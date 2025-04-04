#pragma once

#include "System\\ComPtr.h"
#include "System\\Engine\\Engine.h"

#include "System\\Engine\\Core\\Color.h"

#include "KeyString.h"

#include "System\\Engine\\Core\\XMFLOATHelper.h"

//-----------------------------------------------------------------------------
// 複数のシーンで共用するデータ
//-----------------------------------------------------------------------------
namespace Common
{
	//-----------------------------------------------------------------------------
	// シーンを追加した際に定数を追加します。
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
	// 複数のシーンで共用するデータを構造体にまとめます。
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
