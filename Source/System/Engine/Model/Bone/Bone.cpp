#include "Bone.h"

using namespace DirectX;

Bone::Bone(std::string boneName, XMMATRIX offset)
	: m_position(0.0f, 0.0f, 0.0f)
	, m_rotation(0.0f, 0.0f, 0.0f, 1.0f)
	, m_scale(1.0f, 1.0f, 1.0f)
	, m_pParentBone(nullptr)
	, m_boneName(boneName)
	, m_boneWorldMatrix(XMMatrixIdentity())
	, m_boneOffset(offset)
	, m_bType(BONETYPE_DEFAULT)
	, m_pRigidBody(nullptr)
	, m_parentBoneIndex(UINT_MAX)
{
	//ボーンのワールド座標を取得
	XMVECTOR boneScale;
	XMVECTOR boneQuatation;
	XMVECTOR bonePosition;
	XMMatrixDecompose(&boneScale, &boneQuatation, &bonePosition, m_boneOffset);

	m_initPosition = XMFLOAT3(XMVectorGetX(bonePosition), XMVectorGetY(bonePosition), XMVectorGetZ(bonePosition));
	m_initRotation = XMFLOAT4(XMVectorGetX(boneQuatation), XMVectorGetY(boneQuatation), XMVectorGetZ(boneQuatation), XMVectorGetW(boneQuatation));
}

void Bone::AddChildBone(Bone* pChildBone, const UINT childBoneIndex)
{
	m_childBones.push_back({ pChildBone, childBoneIndex });
}

void Bone::SetParentBone(Bone* pParentBone, const UINT parentBoneIndex)
{
	m_pParentBone = pParentBone;
	m_parentBoneIndex = parentBoneIndex;
}

const std::string Bone::GetBoneName() const
{
	return m_boneName;
}

DirectX::XMMATRIX& Bone::GetGlobalTransform()
{
	return m_boneWorldMatrix;
}

void Bone::SetGlobalTransform(DirectX::XMMATRIX& globalTransform)
{
	m_boneWorldMatrix = globalTransform;
}

void Bone::UpdateChildTransform()
{

}

BoneManager::BoneManager()
	: m_armatureBone(Bone("Armature", DirectX::XMMatrixIdentity()))
{
}
