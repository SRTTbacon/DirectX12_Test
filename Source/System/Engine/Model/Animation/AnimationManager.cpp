#include "AnimationManager.h"

void AnimationManager::LoadAnimation(std::string animFilePath)
{
	//すでにアニメーションがロード済みなら終了
	if (m_animations.find(animFilePath) != m_animations.end()) {
		return;
	}

	Animation animation;
	animation.Load(animFilePath);
	m_animations[animFilePath] = animation;
}