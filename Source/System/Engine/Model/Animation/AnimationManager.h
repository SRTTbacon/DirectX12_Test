#pragma once
#include "Animation.h"
#include <unordered_map>

class AnimationManager
{
public:
	Animation LoadAnimation(std::string animFilePath);

private:
	std::unordered_map<std::string, Animation> m_animations;
};
