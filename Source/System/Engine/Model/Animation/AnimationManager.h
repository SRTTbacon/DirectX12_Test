#pragma once
#include "Animation.h"
#include <unordered_map>

class AnimationManager
{
public:
	//�A�j���[�V���������[�h
	Animation LoadAnimation(std::string animFilePath);

private:
	//���[�h�ς݂̃A�j���[�V���������i�[
	std::unordered_map<std::string, Animation> m_animations;
};
