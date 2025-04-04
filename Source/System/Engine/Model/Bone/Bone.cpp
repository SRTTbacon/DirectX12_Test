#include "Bone.h"

using namespace DirectX;

Bone::Bone(std::string boneName, const UINT boneIndex, const XMFLOAT3* pModelPosition, const XMFLOAT3* pModelRotation, const XMFLOAT3* pModelScale)
    : m_position(0.0f, 0.0f, 0.0f)
    , m_rotation(0.0f, 0.0f, 0.0f, 1.0f)
    , m_scale(1.0f, 1.0f, 1.0f)
    , m_pParentBone(nullptr)
    , m_boneName(boneName)
    , m_boneWorldMatrix(XMMatrixIdentity())
    , m_boneOffset(XMMatrixIdentity())
    , m_boneType(BONETYPE_DEFAULT)
    , m_parentBoneIndex(UINT_MAX)
    , m_boneIndex(boneIndex)
    , m_pModelPosition(pModelPosition)
    , m_pModelRotation(pModelRotation)
    , m_pModelScale(pModelScale)
{
}

void Bone::SetBoneOffset(const XMMATRIX& offset)
{
    m_boneOffset = offset;
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

void Bone::UpdateGlobalMatix(DirectX::XMMATRIX& globalTransform)
{
    XMMATRIX modelScaleMat = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
    XMMATRIX rotX = XMMatrixRotationX(XMConvertToRadians(m_pModelRotation->x));
    XMMATRIX rotY = XMMatrixRotationY(XMConvertToRadians(m_pModelRotation->y));
    XMMATRIX rotZ = XMMatrixRotationZ(XMConvertToRadians(m_pModelRotation->z));
    XMMATRIX modelRotMat = rotX * rotY * rotZ;
    XMMATRIX modelPosMat = XMMatrixTranslation(m_pModelPosition->x, m_pModelPosition->y, m_pModelPosition->z);
    m_boneWorldMatrix = m_boneOffset * globalTransform * modelScaleMat * modelRotMat * modelPosMat;
}

BoneManager::BoneManager(const XMFLOAT3* pModelPosition, const XMFLOAT3* pModelRotation, const XMFLOAT3* pModelScale)
	: m_armatureBone(Bone("Armature", UINT_MAX, pModelPosition, pModelRotation, pModelScale))
    , m_pModelPosition(pModelPosition)
    , m_pModelRotation(pModelRotation)
    , m_pModelScale(pModelScale)
{
}

Bone* BoneManager::AddBone(const std::string boneName, const UINT boneIndex)
{
    Bone bone(boneName, boneIndex, m_pModelPosition, m_pModelRotation, m_pModelScale);

    //ボーン名とIndexを紐づけ
    m_boneMapping[boneName] = boneIndex;
    //配列に追加
    m_boneInfos.push_back(XMMatrixIdentity());
    m_bones.push_back(bone);

    return &m_bones[m_bones.size() - 1];
}

void BoneManager::UpdateBoneMatrix()
{
	//ボーンの最終位置を決定
	UpdateBoneMatrix(0);
}

void BoneManager::UpdateBoneMatrix(UINT boneIndex)
{
    XMMATRIX parentTransform = XMMatrixIdentity();

    //親ボーンが存在する場合、そのワールド座標を適用
    if (m_bones[boneIndex].GetParentBone()) {
        parentTransform = m_boneInfos[m_bones[boneIndex].GetParentBoneIndex()];
    }
    else {
        Bone& armatureBone = m_armatureBone;
        XMMATRIX armatureScale = XMMatrixScaling(armatureBone.m_scale.x, armatureBone.m_scale.y, armatureBone.m_scale.z);
        XMVECTOR armatureRotVec = XMVectorSet(-armatureBone.m_rotation.x, -armatureBone.m_rotation.z, -armatureBone.m_rotation.y, armatureBone.m_rotation.w);
        XMMATRIX armatureRot = XMMatrixRotationQuaternion(armatureRotVec);

        XMMATRIX armaturePos = XMMatrixTranslation(-armatureBone.m_position.x, -armatureBone.m_position.z, armatureBone.m_position.y);

        XMMATRIX boneTransform = armatureScale * armatureRot * armaturePos;

        //親のワールド変換とローカル変換を合成
        parentTransform = XMMatrixTranspose(boneTransform);
    }

    //ボーンの最終的な位置、回転、スケールを決定

    Bone& bone = m_bones[boneIndex];

    XMMATRIX scale = XMMatrixScaling(bone.m_scale.x, bone.m_scale.y, bone.m_scale.z);
    XMVECTOR rotVec = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

    //Unityとの座標系の違いを修正
    switch (bone.m_boneType)
    {
    case BONETYPE_DEFAULT:
        rotVec = XMVectorSet(-bone.m_rotation.x, -bone.m_rotation.z, bone.m_rotation.y, bone.m_rotation.w);
        break;

    case BONETYPE_LEFT_ARM:
        rotVec = XMVectorSet(bone.m_rotation.y, bone.m_rotation.x, bone.m_rotation.z, bone.m_rotation.w);
        break;

    case BONETYPE_LEFT_LEG:
        rotVec = XMVectorSet(-bone.m_rotation.x, bone.m_rotation.z, -bone.m_rotation.y, bone.m_rotation.w);
        break;

    case BONETYPE_RIGHT_ARM:
        rotVec = XMVectorSet(-bone.m_rotation.y, -bone.m_rotation.x, bone.m_rotation.z, bone.m_rotation.w);
        break;

    case BONETYPE_RIGHT_LEG:
        rotVec = XMVectorSet(bone.m_rotation.x, -bone.m_rotation.z, bone.m_rotation.y, -bone.m_rotation.w);
        break;
    }

    XMMATRIX rot = XMMatrixRotationQuaternion(rotVec);

    XMMATRIX pos = XMMatrixTranslation(bone.m_position.x, bone.m_position.z, bone.m_position.y);
    XMMATRIX offsetBack2 = bone.GetBoneOffset();
    XMMATRIX offsetBack = XMMatrixIdentity();

    //位置のみ抽出
    offsetBack.r[3].m128_f32[0] = offsetBack2.r[3].m128_f32[0];
    offsetBack.r[3].m128_f32[1] = offsetBack2.r[3].m128_f32[1];
    offsetBack.r[3].m128_f32[2] = offsetBack2.r[3].m128_f32[2];

    XMMATRIX boneTransform = scale * XMMatrixInverse(nullptr, offsetBack) * rot * offsetBack * pos;

    //親のワールド変換とローカル変換を合成
    XMMATRIX finalTransform = parentTransform * XMMatrixTranspose(boneTransform);
    XMMATRIX finalTransformNonTranspose = XMMatrixTranspose(finalTransform);

    //シェーダーに送るワールド座標に代入
    m_boneInfos[boneIndex] = finalTransform;
    bone.UpdateGlobalMatix(finalTransformNonTranspose);

    //子ボーンにも変換を伝播
    for (UINT i = 0; i < bone.GetChildBoneCount(); i++) {
        UpdateBoneMatrix(bone.GetChildBoneIndex(i));
    }
}
