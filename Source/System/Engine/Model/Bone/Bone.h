#pragma once
#include "..\\..\\Core\\XMFLOATHelper.h"
#include <unordered_map>
#include <string>
#include <Windows.h>

#define _PARENT_BONE_NONE UINT_MAX

enum BoneType
{
    BONETYPE_DEFAULT,
    BONETYPE_LEFT_ARM,
    BONETYPE_LEFT_LEG,
    BONETYPE_LEFT_HAIR,
    BONETYPE_RIGHT_ARM,
    BONETYPE_RIGHT_LEG,
    BONETYPE_RIGHT_HAIR
};

class Bone
{
public:
    //�R���X�g���N�^
    //���� : �{�[�����A�{�[���̃I�t�Z�b�g
    Bone(const std::string boneName, const UINT boneIndex, const DirectX::XMFLOAT3* pModelPosition, const DirectX::XMFLOAT3* pModelRotation, const DirectX::XMFLOAT3* pModelScale);

    //T�|�[�Y(�܂���A�|�[�Y)���̃{�[���̈ʒu�A�p�x��ݒ�
    void SetBoneOffset(const DirectX::XMMATRIX& offset);

    //�q�{�[����ǉ�
    //���� : �q�{�[���̃|�C���^, �q�{�[�������݂���C���f�b�N�X
    void AddChildBone(Bone* pChildBone, const UINT childBoneIndex);

    //�e�{�[����ݒ�
    //���� : �e�{�[�������݂���C���f�b�N�X
    void SetParentBone(Bone* pParentBone, const UINT parentBoneIndex);

    //BoneManager������s�����
    //�V�F�[�_�[�ɑ��M����郏�[���h�}�g���b�N�X��CPU�ň�����`���ɍX�V
    void UpdateGlobalMatix(DirectX::XMMATRIX& globalTransform);

public:
    inline const std::string GetBoneName() const
    {
        return m_boneName;
    }
    inline UINT GetChildBoneCount() const
    {
        return static_cast<UINT>(m_childBones.size());
    }
    inline UINT GetChildBoneIndex(UINT index) const
    {
        return m_childBones[index].boneIndex;
    }
    inline Bone* GetChildBone(UINT index) const
    {
        return m_childBones[index].pBone;
    }
    inline Bone* GetParentBone() const
    {
        return m_pParentBone;
    }
    inline UINT GetParentBoneIndex() const
    {
        return m_parentBoneIndex;
    }
    //�{�[���̃C���f�b�N�X���擾
    inline UINT GetBoneIndex() const
    {
        return m_boneIndex;
    }
    inline DirectX::XMMATRIX GetBoneOffset() const
    {
        return m_boneOffset;
    }
    inline DirectX::XMMATRIX GetBoneOffsetWithModelMatrix() const
    {
        DirectX::XMMATRIX temp = m_boneOffset;
        temp *= DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(m_pModelRotation->x));
        temp *= DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(m_pModelRotation->y));
        temp *= DirectX::XMMatrixRotationZ(DirectX::XMConvertToRadians(m_pModelRotation->z));
        return temp;
    }
    inline DirectX::XMMATRIX GetGlobalTransform() const
    {
        return m_boneWorldMatrix;
    }

public:
    DirectX::XMFLOAT3 m_position;           //�{�[���̈ʒu
    DirectX::XMFLOAT4 m_rotation;           //�{�[���̉�] (���W�A���p)
    DirectX::XMFLOAT3 m_scale;              //�{�[���̃X�P�[��

    BoneType m_boneType;

private:
    struct ChildBone
    {
        Bone* pBone;
        unsigned int boneIndex;
    };

    const std::string m_boneName;           //�{�[����(�s��)

    std::vector<ChildBone> m_childBones;    //�q�{�[��
    Bone* m_pParentBone;                    //�e�{�[��

    UINT m_parentBoneIndex;
    UINT m_boneIndex;

    DirectX::XMMATRIX m_boneWorldMatrix;
    DirectX::XMMATRIX m_boneOffset;

    const DirectX::XMFLOAT3* m_pModelPosition;
    const DirectX::XMFLOAT3* m_pModelRotation;
    const DirectX::XMFLOAT3* m_pModelScale;
};

//------------------------------
//------�{�[���̊Ǘ��N���X------
//------------------------------

class BoneManager
{
public:
    std::vector<Bone> m_bones;                              //�{�[�����
    std::vector<DirectX::XMMATRIX> m_boneInfos;             //�V�F�[�_�[�ɑ��M����{�[���̃}�g���b�N�X
    std::unordered_map<std::string, UINT> m_boneMapping;    //�{�[��������C���f�b�N�X���擾

    Bone m_armatureBone;                                    //���[�g�{�[��

    BoneManager(const DirectX::XMFLOAT3* pModelPosition, const DirectX::XMFLOAT3* pModelRotation, const DirectX::XMFLOAT3* pModelScale);

    Bone* AddBone(const std::string boneName, const UINT boneIndex);

    //���ׂẴ{�[���̃��[���h���W���v�Z
    void UpdateBoneMatrix();
    //����̃{�[���̃��[���h���W���v�Z
    void UpdateBoneMatrix(UINT boneIndex);

public: //�Q�b�^�[
    //�{�[�������݂��邩�ǂ���
    inline bool Exist(std::string boneName) const
    {
        if (m_boneMapping.find(boneName) == m_boneMapping.end()) {
            return false;
        }

        return true;
    }

    //�{�[��������{�[���N���X���擾
    inline Bone* GetBone(std::string boneName)
    {
        if (!Exist(boneName)) {
            return nullptr;
        }

        UINT boneIndex = m_boneMapping[boneName];
        return &m_bones[boneIndex];
    }

    //���ׂẴ{�[�������擾
    inline std::vector<std::string> GetBoneNames()
    {
        std::vector<std::string> boneNames;
        for (Bone& bone : m_bones) {
            boneNames.push_back(bone.GetBoneName());
        }
        return boneNames;
    }

private:
    const DirectX::XMFLOAT3* m_pModelPosition;
    const DirectX::XMFLOAT3* m_pModelRotation;
    const DirectX::XMFLOAT3* m_pModelScale;
};