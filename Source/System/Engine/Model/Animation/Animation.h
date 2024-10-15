#pragma once
#include "..\\..\\Core\\BinaryFile\\BinaryReader.h"
#include <DirectXMath.h>
#include <vector>

//�{�[�����Ƃ̈ʒu�A��]
struct BoneAnimation
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 rotation;
};
//�t���[��
struct AnimationFrame
{
	float time;
	std::vector<BoneAnimation> animations;
};

//�A�j���[�V����
class Animation
{
public:
	void Load(std::string animFilePath);

	AnimationFrame* GetFrame(float nowAnimTime);
	bool IsLastFrame(AnimationFrame* pAnimFrame);

	std::vector<std::string> boneMapping;
	std::vector<AnimationFrame> m_frames;
};