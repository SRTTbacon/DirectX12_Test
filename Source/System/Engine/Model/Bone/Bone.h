#pragma once
#include <DirectXMath.h>
#include <string>
#include <vector>

class Bone
{
public:
    //コンストラクタ
    //引数 : ボーン名、ボーンのオフセット
    Bone(const std::string boneName, DirectX::XMMATRIX offset);

    //子ボーンを追加
    //引数 : 子ボーンのポインタ, 子ボーンが存在するインデックス
    void AddChildBone(Bone* pChildBone, const unsigned int childBoneIndex);

    //親ボーンを設定
    //引数 : 親ボーンが存在するインデックス
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
    DirectX::XMFLOAT3 m_position;           //ボーンの位置
    DirectX::XMFLOAT3 m_rotation;           //ボーンの回転 (デグリー角)
    DirectX::XMFLOAT3 m_scale;              //ボーンのスケール
    DirectX::XMMATRIX m_boneOffset;         //Tポーズ時から見て原点との差
    bool bInited = false;

private:
    struct ChildBone
    {
        Bone* pBone;
        unsigned int boneIndex;
    };

    const std::string m_boneName;           //ボーン名(不変)

    std::vector<ChildBone> m_childBones;         //子ボーン
    unsigned int m_parentBoneIndex;                 //親ボーンのインデックス
};