#pragma once
#include <thread>
#include <future>
#include <mutex>

#include "Model.h"
#include "Animation\\Animation.h"
#include "..\\..\\Main\\Utility.h"
#include "..\\Engine.h"
#include "..\\Core\\BinaryFile\\BinaryCompression.h"

//ボーンが存在するモデルを読み込む
//シェイプキー(UnityのBlendShape)に対応
//Unityで動くアニメーションに対応 (エディター拡張で*.anim->*.hscに変換する必要あり)

class Engine;

class Character : public Model
{
public:

    //ヒューマノイドモデル
    struct HumanoidMesh {
        Mesh* pMesh;                                            //メッシュ情報ののポインタ
        std::unordered_map<std::string, UINT> shapeMapping;     //シェイプキー名からインデックスを取得
        std::vector<XMFLOAT3> shapeDeltas;                      //各頂点のシェイプキー適応後の位置を格納 (頂点の数 * シェイプキーの数だけ要素数を確保)
        std::vector<float> shapeWeights;                        //メッシュごとのシェイプキーのウェイト一覧
        std::string meshName;                                   //メッシュ名
        UINT vertexCount;                                       //メッシュの頂点数

        HumanoidMesh();
    };

    //コンストラクタ
    //基本的にエンジンで実行
    //引数 : std::string FBXファイルのパス, Camera* カメラクラスのポインタ, ID3D12Device* デバイス, ID3D12GraphicsCommandList* コマンドリスト, Camera* カメラ, 
    //          DirectionalLight* ディレクショナルライト, ID3D12Resource* 影が書き込まれるバッファ
    Character(const std::string modelFile, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const Camera* pCamera, DirectionalLight* pDirectionalLight,
        ID3D12Resource* pShadowMapBuffer);
    ~Character();

    //アニメーションを追加
    //戻り値 : 追加されたアニメーションのインデックス
    UINT AddAnimation(Animation animation);

    //位置関係やアニメーションを更新
    void Update(UINT backBufferIndex);

    //ボーンの位置を更新
    void UpdateBonePosition(std::string boneName, XMFLOAT3& position);
    //ボーンの回転を変更
    void UpdateBoneRotation(std::string boneName, XMFLOAT4& rotation);
    //ボーンのスケールを変更
    void UpdateBoneScale(std::string boneName, XMFLOAT3& scale);

    //シェイプキーのウェイトを更新
    void SetShapeWeight(const std::string shapeName, float weight);

public: //ゲッター関数

    //ボーン情報を取得
    inline Bone* GetBone(std::string boneName)
    {
        if (m_boneManager.m_boneMapping.find(boneName) == m_boneManager.m_boneMapping.end()) {
            return nullptr;
        }
        return &m_boneManager.m_bones[m_boneManager.m_boneMapping[boneName]];
    }
    //ボーンのオフセットを取得
    inline XMFLOAT3 GetBoneOffset(std::string boneName)
    {
        if (m_boneManager.m_boneMapping.find(boneName) == m_boneManager.m_boneMapping.end()) {
            return XMFLOAT3(0.0f, 0.0f, 0.0f);
        }

        XMMATRIX m = m_boneManager.m_bones[m_boneManager.m_boneMapping[boneName]].m_boneOffset;
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
    inline float GetShapeWeight(std::string meshName, UINT shapeIndex)
    {
        for (HumanoidMesh& mesh : m_humanoidMeshes) {
            if (mesh.meshName != meshName)
                continue;

            return mesh.shapeWeights[shapeIndex];
        }

        return 0.0f;
    }

    inline std::vector<HumanoidMesh>& GetHumanMeshes()
    {
        return m_humanoidMeshes;
    }

	inline HumanoidMesh* GetHumanMesh(std::string meshName)
	{
		for (HumanoidMesh& mesh : m_humanoidMeshes) {
			if (mesh.meshName == meshName) {
				return &mesh;
			}
		}
		return nullptr;
	}

public: //パブリック変数
    BoneManager m_boneManager;  //ボーンに関する情報を保持する構造体   

    float m_animationSpeed;     //アニメーション速度。1.0fが通常速度
    float m_nowAnimationTime;   //現在のアニメーション時間

private:
    //シェーダーに渡す頂点情報
    struct Vertex {
        XMFLOAT3 position;      //頂点の位置 (Tポーズ)
        XMFLOAT4 boneWeights;   //ボーンの影響度
        UINT boneIDs[4];        //4つのボーンから影響を受ける
        XMFLOAT3 normal;        //法線
        XMFLOAT2 texCoords;     //UV
		XMFLOAT3 tangent;       //接線
		XMFLOAT3 bitangent;     //従法線
        UINT vertexID;          //頂点ID
    };
    //シェーダーに渡す頂点数の情報 (構造体で渡す必要?)
    struct Contents {
        UINT vertexCount;
        UINT shapeCount;
    };

    //ボーンが存在するFBXファイルをロード
    void LoadFBX(const std::string& fbxFile);
	//ボーンが存在するHCSファイルをロード
    void LoadHCS(const std::string& hcsFile);

    //ノードを読み込み
    void ProcessNode(const aiScene* pScene, const aiNode* pNode);
    //メッシュ情報を.fbxファイルから取得
    Mesh* ProcessMesh(aiMesh* mesh, HumanoidMesh& humanoidMesh);
    //メッシュ情報を.hcsファイルから取得
    Mesh* ProcessMesh(BinaryReader& br, HumanoidMesh& humanoidMesh);

    //各ボーンのワールド座標を算出
    void CalculateBoneTransforms(const aiNode* node, const XMMATRIX& parentTransform);

    //ボーン情報を.fbxから取得
    void LoadBones(aiMesh* mesh, std::vector<Vertex>& vertices);
    //ボーン情報を.hcsファイルから取得
    void LoadBones(BinaryReader& br);
    //ボーンの親子関係を取得(.fbxのみ)
    void LoadBoneFamily(const aiNode* node);

    //シェイプキーと頂点の関係を取得(.fbxのみ)
    void LoadShapeKey(const aiMesh* mesh, std::vector<Vertex>& vertices, HumanoidMesh& humanoidMesh);
    //HumanoidMeshの内容を取得 (.hcsのみ)
    void LoadHumanoidMesh(BinaryReader& br);

    //ファイルからテクスチャを作成
    void SetTexture(const std::string nameOnly);

    //シェーダーに渡すボーンの座標を計算
    void UpdateBoneTransform(UINT boneIndex, XMMATRIX& parentMatrix);
    //シェーダーに渡すすべてのボーンの座標を計算
    void UpdateBoneTransform();
    //シェイプキーのウェイトを更新
    void UpdateShapeKeys();
    //アニメーションを更新
    void UpdateAnimation();

    //必要なバッファを作成
    void CreateBuffer(Mesh* pMesh, std::vector<Vertex>& vertices, std::vector<UINT>& indices, HumanoidMesh& humanoidMesh);
    //ヒューマノイドに必要なバッファを作成
    void CreateHumanoidMeshBuffer(HumanoidMesh& humanoidMesh);
    //シェイプキーのデータをテクスチャとして作成 (テクスチャにすることで処理速度大幅UP)
    void CreateShapeDeltasTexture(HumanoidMesh& humanoidMesh);

private:  //プライベート変数
    std::vector<HumanoidMesh> m_humanoidMeshes;             //ヒューマノイド用のメッシュ情報
    std::vector<Animation> m_animations;                    //アニメーション情報

    int m_nowAnimationIndex;                                //現在再生中のアニメーション番号
    bool bHCSFile;
};