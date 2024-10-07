#pragma once
#include "..\\..\\Core\\BinaryFile\\BinaryReader.h"
#include "..\\..\\Engine.h"

struct BoneAnimation
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 rotation;
};
struct AnimationFrame
{
	float time;
	std::vector<BoneAnimation> anim;
};

class Animation
{
public:
	Animation(std::string animFilePath);

	std::vector<BoneAnimation>& Update();

	std::vector<std::string> boneMapping;
	std::vector<AnimationFrame> m_frames;

	float m_nowAnimTime;
};