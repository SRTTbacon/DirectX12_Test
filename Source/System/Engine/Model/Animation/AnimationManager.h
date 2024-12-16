#pragma once
#include "Animation.h"
#include <unordered_map>

class AnimationManager
{
public:
	//アニメーションをロード
	Animation LoadAnimation(std::string animFilePath);

private:
	//ロード済みのアニメーション情報を格納
	std::unordered_map<std::string, Animation> m_animations;
};
