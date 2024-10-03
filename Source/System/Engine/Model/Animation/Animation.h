#pragma once
#include "..\\..\\Core\\BinaryFile\\BinaryReader.h"
#include <DirectXMath.h>
#include <vector>

struct BoneAnimation
{
	std::string boneName;
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 initPosition;
	DirectX::XMFLOAT3 rotation;
};

class Animation
{
public:
	Animation(std::string animFilePath);

	std::vector<BoneAnimation> m_boneAnim;
};