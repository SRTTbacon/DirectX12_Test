#pragma once
#include "Model.h"
#include "..\\..\\Main\\Utility.h"

class Character : public Model
{
public:
	Character(const std::string fbxFile, const Camera* pCamera);

    void Update();

    //ボーンの位置を更新
    void UpdateBonePosition(std::string boneName, XMFLOAT3& position);
    //ボーンの回転を変更
    void UpdateBoneRotation(std::string boneName, XMFLOAT4& rotation);
    //ボーンのスケールを変更
    void UpdateBoneScale(std::string boneName, XMFLOAT3& scale);

    //シェイプキーのウェイトを更新
    void SetShapeWeight(UINT shapeIndex, float weight);
    void SetShapeWeight(const std::string shapeName, float weight);

public: //ゲッター関数 (頻繁に呼び出すものはinline)

    //すべてのボーン名を取得
    std::vector<std::string> GetBoneNames();

    //ボーン情報を取得
    inline Bone* GetBone(std::string boneName)
    {
        if (boneMapping.find(boneName) == boneMapping.end()) {
            return nullptr;
        }
        return &bones[boneMapping[boneName]];
    }
    //ボーンのオフセットを取得
    inline XMFLOAT3 GetBoneOffset(std::string boneName)
    {
        if (boneMapping.find(boneName) == boneMapping.end()) {
            return XMFLOAT3(0.0f, 0.0f, 0.0f);
        }

        XMMATRIX m = bones[boneMapping[boneName]].GetBoneOffset();
        return XMFLOAT3(m.r[3].m128_f32[0], m.r[3].m128_f32[1], m.r[3].m128_f32[2]);
    }

    //シェイプキーのウェイトを取得
    inline float GetShapeWeight(std::string shapeName)
    {
        if (shapeMapping.find(shapeName) == shapeMapping.end())
            return 0.0f;

        return GetShapeWeight(shapeMapping[shapeName]);
    }
    //シェイプキーのウェイトを取得
    inline float GetShapeWeight(UINT shapeIndex)
    {
        if (shapeIndex < 0 || shapeWeights.size() <= shapeIndex)
            return 0.0f;

        return shapeWeights[shapeIndex];
    }

private:
    //プライベート変数
    std::vector<Bone> bones;                            //ボーン情報
    std::unordered_map<std::string, UINT> boneMapping;  //ボーン名からインデックスを取得

    std::unordered_map<std::string, UINT> shapeMapping; //シェイプキー名からインデックスを取得

private:
    //シェーダーに渡す頂点情報
    struct Vertex {
        XMFLOAT3 Position;
        XMFLOAT3 Normal;
        XMFLOAT2 TexCoords;
        XMFLOAT4 Color; // 頂点色
        XMFLOAT4 BoneWeights;
        UINT BoneIDs[4];
        XMFLOAT3 ShapePosition;
        UINT ShapeID;
    };

    //ボーンが存在するFBXファイルをロード
    void LoadFBX(const std::string& fbxFile);

    void ProcessNode(const aiScene* scene, aiNode* node);   //ノードを読み込み
    Mesh ProcessMesh(const aiScene* scene, aiMesh* mesh);   //メッシュ情報を読み込み
    void LoadBones(const aiScene* scene, Mesh& meshStruct, aiMesh* mesh, std::vector<Vertex>& vertices);    //ボーン情報を取得
    void LoadBoneFamily(const aiNode* node);                //ボーンの親子関係を取得
    void LoadShapeKey(const aiMesh* mesh, std::vector<Vertex>& vertices);   //シェイプキーと頂点の関係を取得
    void UpdateBoneTransform(UINT boneIndex, XMMATRIX& parentMatrix);   //シェーダーに渡すボーンの座標を計算
    void UpdateBoneTransform();                                         //シェーダーに渡すボーンの座標を計算
    void UpdateShapeKeys();
};