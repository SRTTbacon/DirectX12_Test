#include "Bone.h"

Bone::Bone(std::string boneName, DirectX::XMMATRIX offset)
	: m_position(0.0f, 0.0f, 0.0f)
	, m_rotation(0.0f, 0.0f, 0.0f)
	, m_scale(1.0f, 1.0f, 1.0f)
	, m_parentBoneIndex(UINT32_MAX)
	, m_boneName(boneName)
	, m_boneOffset(offset)
{
}

void Bone::AddChildBone(Bone* pChildBone, const unsigned int childBoneIndex)
{
	m_childBones.push_back({ pChildBone, childBoneIndex });
}

void Bone::SetParentBone(const unsigned int parentBoneIndex)
{
	m_parentBoneIndex = parentBoneIndex;
}

const std::string Bone::GetBoneName() const
{
	return m_boneName;
}