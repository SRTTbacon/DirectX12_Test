#include "AnimationManager.h"

Animation* AnimationManager::LoadAnimation(std::string animFilePath)
{
	//すでにアニメーションがロード済みなら終了
	if (m_animations.find(animFilePath) != m_animations.end()) {
		return &m_animations[animFilePath];
	}

	Animation animation;
	animation.Load(animFilePath);
	m_animations[animFilePath] = animation;
	return &m_animations[animFilePath];
}
