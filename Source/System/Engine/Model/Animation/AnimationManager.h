#pragma once
#include "Animation.h"
#include <unordered_map>

class AnimationManager
{
public:
	void LoadAnimation(std::string animFilePath);

	std::unordered_map<std::string, Animation> m_animations;
};