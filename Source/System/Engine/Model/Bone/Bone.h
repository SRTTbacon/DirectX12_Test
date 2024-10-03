#pragma once
#include <DirectXMath.h>
#include <string>
#include <vector>

class Bone
{
public:
    //�R���X�g���N�^
    //���� : �{�[�����A�{�[���̃I�t�Z�b�g
    Bone(const std::string boneName, DirectX::XMMATRIX offset);

    //�q�{�[����ǉ�
    //���� : �q�{�[���̃|�C���^, �q�{�[�������݂���C���f�b�N�X
    void AddChildBone(Bone* pChildBone, const unsigned int childBoneIndex);

    //�e�{�[����ݒ�
    //���� : �e�{�[�������݂���C���f�b�N�X
    void SetParentBone(const unsigned int parentBoneIndex);

    const std::string GetBoneName() const;

    inline const unsigned int GetChildBoneCount() const
    {
        return (unsigned int)m_childBones.size();
    }
    inline const unsigned int GetChildBoneIndex(unsigned int index) const
    {
        return m_childBones[index].boneIndex;
    }
    inline Bone* GetChildBone(unsigned int index) const
    {
        return m_childBones[index].pBone;
    }
    inline const unsigned int GetParentBoneIndex() const
    {
        return m_parentBoneIndex;
    }
    inline const DirectX::XMMATRIX GetBoneOffset() const
    {
        return m_boneOffset;
    }

public:
    DirectX::XMFLOAT3 m_position;           //�{�[���̈ʒu
    DirectX::XMFLOAT3 m_rotation;           //�{�[���̉�] (�f�O���[�p)
    DirectX::XMFLOAT3 m_scale;              //�{�[���̃X�P�[��
    DirectX::XMMATRIX m_boneOffset;         //T�|�[�Y�����猩�Č��_�Ƃ̍�
    bool bInited = false;

private:
    struct ChildBone
    {
        Bone* pBone;
        unsigned int boneIndex;
    };

    const std::string m_boneName;           //�{�[����(�s��)

    std::vector<ChildBone> m_childBones;         //�q�{�[��
    unsigned int m_parentBoneIndex;                 //�e�{�[���̃C���f�b�N�X
};