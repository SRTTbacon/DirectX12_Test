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
    //コンストラクタ
    //引数 : ボーン名、ボーンのオフセット
    Bone(const std::string boneName, const UINT boneIndex, const DirectX::XMFLOAT3* pModelPosition, const DirectX::XMFLOAT3* pModelRotation, const DirectX::XMFLOAT3* pModelScale);

    //Tポーズ(またはAポーズ)時のボーンの位置、角度を設定
    void SetBoneOffset(const DirectX::XMMATRIX& offset);

    //子ボーンを追加
    //引数 : 子ボーンのポインタ, 子ボーンが存在するインデックス
    void AddChildBone(Bone* pChildBone, const UINT childBoneIndex);

    //親ボーンを設定
    //引数 : 親ボーンが存在するインデックス
    void SetParentBone(Bone* pParentBone, const UINT parentBoneIndex);

    //BoneManagerから実行される
    //シェーダーに送信されるワールドマトリックスをCPUで扱える形式に更新
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
    //ボーンのインデックスを取得
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
    DirectX::XMFLOAT3 m_position;           //ボーンの位置
    DirectX::XMFLOAT4 m_rotation;           //ボーンの回転 (ラジアン角)
    DirectX::XMFLOAT3 m_scale;              //ボーンのスケール

    BoneType m_boneType;

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
    UINT m_boneIndex;

    DirectX::XMMATRIX m_boneWorldMatrix;
    DirectX::XMMATRIX m_boneOffset;

    const DirectX::XMFLOAT3* m_pModelPosition;
    const DirectX::XMFLOAT3* m_pModelRotation;
    const DirectX::XMFLOAT3* m_pModelScale;
};

//------------------------------
//------ボーンの管理クラス------
//------------------------------

class BoneManager
{
public:
    std::vector<Bone> m_bones;                              //ボーン情報
    std::vector<DirectX::XMMATRIX> m_boneInfos;             //シェーダーに送信するボーンのマトリックス
    std::unordered_map<std::string, UINT> m_boneMapping;    //ボーン名からインデックスを取得

    Bone m_armatureBone;                                    //ルートボーン

    BoneManager(const DirectX::XMFLOAT3* pModelPosition, const DirectX::XMFLOAT3* pModelRotation, const DirectX::XMFLOAT3* pModelScale);

    Bone* AddBone(const std::string boneName, const UINT boneIndex);

    //すべてのボーンのワールド座標を計算
    void UpdateBoneMatrix();
    //特定のボーンのワールド座標を計算
    void UpdateBoneMatrix(UINT boneIndex);

public: //ゲッター
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

private:
    const DirectX::XMFLOAT3* m_pModelPosition;
    const DirectX::XMFLOAT3* m_pModelRotation;
    const DirectX::XMFLOAT3* m_pModelScale;
};