#pragma once
#include "..\\..\\Core\\BinaryFile\\BinaryReader.h"
#include <DirectXMath.h>
#include <vector>

//ボーンごとの位置、回転
struct BoneAnimation
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 rotation;
};
//フレーム
struct AnimationFrame
{
	float time;
	std::vector<BoneAnimation> animations;
};

//アニメーション
class Animation
{
public:
	void Load(std::string animFilePath);

	AnimationFrame* GetFrame(float nowAnimTime);
	bool IsLastFrame(AnimationFrame* pAnimFrame);

	std::vector<std::string> boneMapping;
	std::vector<AnimationFrame> m_frames;
};