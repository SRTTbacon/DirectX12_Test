#include "AnimationManager.h"

void AnimationManager::LoadAnimation(std::string animFilePath)
{
	//���łɃA�j���[�V���������[�h�ς݂Ȃ�I��
	if (m_animations.find(animFilePath) != m_animations.end()) {
		return;
	}

	Animation animation;
	animation.Load(animFilePath);
	m_animations[animFilePath] = animation;
}