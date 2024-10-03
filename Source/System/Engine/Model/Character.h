#pragma once
#include "Model.h"

class Character : public Model
{
public:
	Character(const std::string fbxFile, const Camera* pCamera);

    void Update();

    //ボーンの位置を更新
    void UpdateBonePosition(std::string boneName, XMFLOAT3& position);
    //ボーンの回転を変更
    void UpdateBoneRotation(std::string boneName, XMFLOAT3& rotation);
    //ボーンのスケールを変更
    void UpdateBoneScale(std::string boneName, XMFLOAT3& scale);

    std::vector<std::string> GetBoneNames();

    inline Bone* GetBone(std::string boneName)
    {
        if (boneMapping.find(boneName) == boneMapping.end()) {
            return nullptr;
        }
        return &bones[boneMapping[boneName]];
    }
    inline XMFLOAT3 GetBoneOffset(std::string boneName)
    {
        XMMATRIX m = bones[boneMapping[boneName]].GetBoneOffset();
        return XMFLOAT3(m.r[3].m128_f32[0], m.r[3].m128_f32[1], m.r[3].m128_f32[2]);
    }

private:
    //シェーダーに渡す頂点情報
    struct Vertex {
        XMFLOAT3 Position;
        XMFLOAT3 Normal;
        XMFLOAT2 TexCoords;
        XMFLOAT4 Color; // 頂点色
        XMFLOAT4 BoneWeights;
        UINT BoneIDs[4];
    };

    //ボーンが存在するFBXファイルをロード
    void LoadFBX(const std::string& fbxFile);

    void ProcessNode(const aiScene* scene, aiNode* node);   //ノードを読み込み
    Mesh ProcessMesh(const aiScene* scene, aiMesh* mesh);   //メッシュ情報を読み込み
    void LoadBones(const aiScene* scene, Mesh& meshStruct, aiMesh* mesh, std::vector<Vertex>& vertices);    //ボーン情報を取得
    void LoadBoneFamily(const aiNode* node);                //ボーンの親子関係を取得
    void UpdateBoneTransform(UINT boneIndex, XMMATRIX& parentMatrix);   //シェーダーに渡すボーンの座標を計算
    void UpdateBoneTransform();                                         //シェーダーに渡すボーンの座標を計算
};