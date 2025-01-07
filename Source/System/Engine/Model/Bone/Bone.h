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
    //コンストラクタ
    //引数 : ボーン名、ボーンのオフセット
    Bone(const std::string boneName, DirectX::XMMATRIX offset);

    //子ボーンを追加
    //引数 : 子ボーンのポインタ, 子ボーンが存在するインデックス
    void AddChildBone(Bone* pChildBone, const UINT childBoneIndex);

    //親ボーンを設定
    //引数 : 親ボーンが存在するインデックス
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

    DirectX::XMFLOAT3 m_position;           //ボーンの位置
    DirectX::XMFLOAT4 m_rotation;           //ボーンの回転 (デグリー角)
    DirectX::XMFLOAT3 m_scale;              //ボーンのスケール
    DirectX::XMMATRIX m_boneOffset;         //Tポーズ時から見て原点との差

    BoneType m_bType;

private:
    struct ChildBone
    {
        Bone* pBone;
        unsigned int boneIndex;
    };

    const std::string m_boneName;           //ボーン名(不変)

    std::vector<ChildBone> m_childBones;    //子ボーン
    Bone* m_pParentBone;                    //親ボーン

    UINT m_parentBoneIndex;

    DirectX::XMMATRIX m_boneWorldMatrix;
    DirectX::XMFLOAT3 m_initPosition;
    DirectX::XMFLOAT4 m_initRotation;
};

struct BoneManager
{
    std::vector<Bone> m_bones;                              //ボーン情報
    std::vector<DirectX::XMMATRIX> m_boneInfos;             //シェーダーに送信するボーンのマトリックス
    std::unordered_map<std::string, UINT> m_boneMapping;    //ボーン名からインデックスを取得
    std::unordered_map<std::string, DirectX::XMMATRIX> m_finalBoneTransforms;

    Bone m_armatureBone;                                    //ルートボーン

    BoneManager();

    //ボーンが存在するかどうか
    inline bool Exist(std::string boneName) const
    {
        if (m_boneMapping.find(boneName) == m_boneMapping.end()) {
            return false;
        }

        return true;
    }

    //ボーン名からボーンクラスを取得
    inline Bone* GetBone(std::string boneName)
    {
        if (!Exist(boneName)) {
            return nullptr;
        }

        UINT boneIndex = m_boneMapping[boneName];
        return &m_bones[boneIndex];
    }

    //すべてのボーン名を取得
    inline std::vector<std::string> GetBoneNames()
    {
        std::vector<std::string> boneNames;
        for (Bone& bone : m_bones) {
            boneNames.push_back(bone.GetBoneName());
        }
        return boneNames;
    }
};