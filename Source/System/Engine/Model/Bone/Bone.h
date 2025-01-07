#pragma once
#include <DirectXMath.h>
#include <unordered_map>
#include <string>
#include <Windows.h>

#include <btBulletDynamicsCommon.h>

#define _PARENT_BONE_NONE UINT_MAX

enum BoneType
{
    BONETYPE_DEFAULT,
    BONETYPE_LEFT_ARM,
    BONETYPE_LEFT_LEG,
    BONETYPE_RIGHT_ARM,
    BONETYPE_RIGHT_LEG
};

class Bone
{
public:
    //�R���X�g���N�^
    //���� : �{�[�����A�{�[���̃I�t�Z�b�g
    Bone(const std::string boneName, DirectX::XMMATRIX offset);

    //�q�{�[����ǉ�
    //���� : �q�{�[���̃|�C���^, �q�{�[�������݂���C���f�b�N�X
    void AddChildBone(Bone* pChildBone, const UINT childBoneIndex);

    //�e�{�[����ݒ�
    //���� : �e�{�[�������݂���C���f�b�N�X
    void SetParentBone(Bone* pParentBone, const UINT parentBoneIndex);

    const std::string GetBoneName() const;

    DirectX::XMMATRIX& GetGlobalTransform();
    void SetGlobalTransform(DirectX::XMMATRIX& globalTransform);

    void UpdateChildTransform();

    inline UINT GetChildBoneCount() const
    {
        return (unsigned int)m_childBones.size();
    }
    inline UINT GetChildBoneIndex(unsigned int index) const
    {
        return m_childBones[index].boneIndex;
    }
    inline Bone* GetChildBone(unsigned int index) const
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
    inline DirectX::XMFLOAT3 GetInitPosition() const
    {
        return m_initPosition;
    }
    inline DirectX::XMFLOAT4 GetInitRotation() const
    {
        return m_initRotation;
    }

public:
    btRigidBody* m_pRigidBody;

    DirectX::XMFLOAT3 m_position;           //�{�[���̈ʒu
    DirectX::XMFLOAT4 m_rotation;           //�{�[���̉�] (�f�O���[�p)
    DirectX::XMFLOAT3 m_scale;              //�{�[���̃X�P�[��
    DirectX::XMMATRIX m_boneOffset;         //T�|�[�Y�����猩�Č��_�Ƃ̍�

    BoneType m_bType;

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

    DirectX::XMMATRIX m_boneWorldMatrix;
    DirectX::XMFLOAT3 m_initPosition;
    DirectX::XMFLOAT4 m_initRotation;
};

struct BoneManager
{
    std::vector<Bone> m_bones;                              //�{�[�����
    std::vector<DirectX::XMMATRIX> m_boneInfos;             //�V�F�[�_�[�ɑ��M����{�[���̃}�g���b�N�X
    std::unordered_map<std::string, UINT> m_boneMapping;    //�{�[��������C���f�b�N�X���擾
    std::unordered_map<std::string, DirectX::XMMATRIX> m_finalBoneTransforms;

    Bone m_armatureBone;                                    //���[�g�{�[��

    BoneManager();

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
};