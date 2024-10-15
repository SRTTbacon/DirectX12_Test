#pragma once
#include "Model.h"
#include "..\\..\\Main\\Utility.h"

class Character : public Model
{
public:
	Character(const std::string fbxFile, const Camera* pCamera);

    void LoadAnimation(std::string animFile);

    void Update();

    //ボーンの位置を更新
    void UpdateBonePosition(std::string boneName, XMFLOAT3& position);
    //ボーンの回転を変更
    void UpdateBoneRotation(std::string boneName, XMFLOAT4& rotation);
    //ボーンのスケールを変更
    void UpdateBoneScale(std::string boneName, XMFLOAT3& scale);

    //シェイプキーのウェイトを更新
    void SetShapeWeight(const std::string shapeName, float weight);

public: //ゲッター関数 (頻繁に呼び出すものはinline)

    //すべてのボーン名を取得
    std::vector<std::string> GetBoneNames();

    //ボーン情報を取得
    inline Bone* GetBone(std::string boneName)
    {
        if (m_boneMapping.find(boneName) == m_boneMapping.end()) {
            return nullptr;
        }
        return &m_bones[m_boneMapping[boneName]];
    }
    //ボーンのオフセットを取得
    inline XMFLOAT3 GetBoneOffset(std::string boneName)
    {
        if (m_boneMapping.find(boneName) == m_boneMapping.end()) {
            return XMFLOAT3(0.0f, 0.0f, 0.0f);
        }

        XMMATRIX m = m_bones[m_boneMapping[boneName]].GetBoneOffset();
        return XMFLOAT3(m.r[3].m128_f32[0], m.r[3].m128_f32[1], m.r[3].m128_f32[2]);
    }

    //シェイプキーのウェイトを取得
    inline float GetShapeWeight(std::string shapeName)
    {
        //同名のシェイプキーはメッシュ間で共有されているため、最初に見つかったshapeNameのシェイプキーのウェイトを返す
        for (HumanoidMesh& mesh : m_humanoidMeshes) {
            if (mesh.shapeMapping.find(shapeName) == mesh.shapeMapping.end())
                continue;

            return mesh.shapeWeights[mesh.shapeMapping[shapeName]];
        }

        return 0.0f;
    }

public: //パブリック変数
    float m_animationSpeed;

private:
    //シェーダーに渡す頂点情報
    struct Vertex {
        XMFLOAT3 position;      //頂点の位置 (Tポーズ)
        XMFLOAT3 normal;        //法線
        XMFLOAT2 texCoords;     //UV
        XMFLOAT4 color;         //色
        XMFLOAT4 boneWeights;   //ボーンの影響度
        UINT boneIDs[4];        //4つのボーンから影響を受ける
        UINT vertexID;          //頂点ID
    };
    //シェーダーに渡す頂点数の情報 (構造体で渡す必要?)
    struct Contents {
        UINT vertexCount;
        UINT shapeCount;
    };
    //ヒューマノイドモデル
    struct HumanoidMesh {
        Mesh* pMesh;                                            //メッシュ情報ののポインタ
        std::unordered_map<std::string, UINT> shapeMapping;     //シェイプキー名からインデックスを取得
        std::vector<XMFLOAT3> shapeDeltas;                      //各頂点のシェイプキー適応後の位置を格納 (頂点の数 * シェイプキーの数だけ要素数を確保)
        std::vector<float> shapeWeights;                        //メッシュごとのシェイプキーのウェイト一覧
        UINT vertexCount;                                       //メッシュの頂点数
    };

    //ボーンが存在するFBXファイルをロード
    void LoadFBX(const std::string& fbxFile);

    void ProcessNode(const aiScene* scene, aiNode* node);   //ノードを読み込み
    Mesh* ProcessMesh(const aiScene* scene, aiMesh* mesh, HumanoidMesh& humanoidMesh);                       //メッシュ情報を読み込み
    void LoadBones(const aiScene* scene, aiMesh* mesh, std::vector<Vertex>& vertices);    //ボーン情報を取得
    void LoadBoneFamily(const aiNode* node);                //ボーンの親子関係を取得
    void LoadShapeKey(const aiMesh* mesh, std::vector<Vertex>& vertices, HumanoidMesh& humanoidMesh);       //シェイプキーと頂点の関係を取得
    void UpdateBoneTransform(UINT boneIndex, XMMATRIX& parentMatrix);   //シェーダーに渡すボーンの座標を計算
    void UpdateBoneTransform();                                         //シェーダーに渡すボーンの座標を計算
    void UpdateShapeKeys();     //シェイプキーのウェイトを更新
    void UpdateAnimation();     //アニメーションを更新
    void CreateBuffer(Mesh* pMesh, std::vector<Vertex>& vertices, std::vector<UINT>& indices, HumanoidMesh& humanoidMesh);
    void CreateShapeDeltasTexture(HumanoidMesh& humanoidMesh);

private:  //プライベート変数
    std::vector<Bone> m_bones;                              //ボーン情報
    std::vector<XMMATRIX> m_boneInfos;                      //シェーダーに送信するボーンのマトリックス
    std::unordered_map<std::string, UINT> m_boneMapping;    //ボーン名からインデックスを取得
    std::vector<HumanoidMesh> m_humanoidMeshes;             //ヒューマノイド用のメッシュ情報

    Animation* m_pAnimation;                                //アニメーション情報
    float m_nowAnimationTime;                               //現在のアニメーション時間
};