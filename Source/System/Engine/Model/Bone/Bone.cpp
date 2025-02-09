#include "Bone.h"

using namespace DirectX;

Bone::Bone(std::string boneName, XMMATRIX offset, UINT boneIndex)
	: m_position(0.0f, 0.0f, 0.0f)
	, m_initPos(0.0f, 0.0f ,0.0f)
	, m_rotation(0.0f, 0.0f, 0.0f, 1.0f)
	, m_scale(1.0f, 1.0f, 1.0f)
	, m_pParentBone(nullptr)
	, m_boneName(boneName)
	, m_boneWorldMatrix(XMMatrixIdentity())
	, m_boneOffset(offset)
	, m_bType(BONETYPE_DEFAULT)
	, m_pRigidBody(nullptr)
	, m_parentBoneIndex(UINT_MAX)
	, m_boneIndex(boneIndex)
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

BoneManager::BoneManager()
	: m_armatureBone(Bone("Armature", DirectX::XMMatrixIdentity(), UINT_MAX))
{
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

    //Unityとの座標系の違いを修正 (導き出すのにめっっっちゃ時間かかった。なんで手と足と胴体で仕様違うの。)
    switch (bone.m_bType)
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

    //printf("%s -> x=%f, y=%f, z=%f, w=%f\n", bone.GetBoneName().c_str(), XMVectorGetX(rotVec), XMVectorGetY(rotVec), XMVectorGetZ(rotVec), XMVectorGetW(rotVec));

    XMMATRIX rot = XMMatrixRotationQuaternion(rotVec);

    XMMATRIX pos = XMMatrixTranslation(bone.m_position.x, bone.m_position.z, bone.m_position.y);
    XMMATRIX offsetBack2 = m_finalBoneTransforms[bone.GetBoneName()];
    XMMATRIX offsetBack = XMMatrixIdentity();

    //位置のみ抽出
    offsetBack.r[3].m128_f32[0] = offsetBack2.r[3].m128_f32[0];
    offsetBack.r[3].m128_f32[1] = offsetBack2.r[3].m128_f32[1];
    offsetBack.r[3].m128_f32[2] = offsetBack2.r[3].m128_f32[2];

    //XMMATRIX boneTransform = scale * offsetBack * rot * bone.GetBoneOffset() * pos;
    XMMATRIX boneTransform = scale * XMMatrixInverse(nullptr, offsetBack) * rot * offsetBack * pos;

    //親のワールド変換とローカル変換を合成
    XMMATRIX finalTransform = parentTransform * XMMatrixTranspose(boneTransform);

    //シェーダーに送るワールド座標に代入
    m_boneInfos[boneIndex] = finalTransform;
    bone.SetGlobalTransform(offsetBack2);

    //子ボーンにも変換を伝播
    for (UINT i = 0; i < bone.GetChildBoneCount(); i++) {
        UpdateBoneMatrix(bone.GetChildBoneIndex(i));
    }
}
