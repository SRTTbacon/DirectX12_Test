#pragma once
#include <filesystem>

#include "Model.h"
#include "..\\..\\Main\\Utility.h"
#include "..\\Core\\BinaryFile\\BinaryCompression.h"
#include "..\\Core\\ResourceCopy\\ResourceCopy.h"

//ボーンが存在するモデルを読み込む
//シェイプキー(UnityのBlendShape)に対応
//Unityで動くアニメーションに対応 (エディター拡張で*.anim->*.hscに変換する必要あり)

class Character : public Model
{
public:
    //ヒューマノイドモデル
    struct HumanoidMesh {
        Mesh* pMesh;                                            //メッシュ情報のポインタ
        std::unordered_map<std::string, UINT> shapeMapping;     //シェイプキー名からインデックスを取得
        std::vector<XMFLOAT3> shapeDeltas;                      //各頂点のシェイプキー適応後の位置を格納 (頂点の数 * シェイプキーの数だけ要素数を確保)
        std::vector<float> shapeWeights;                        //メッシュごとのシェイプキーのウェイト一覧
        UINT vertexCount;                                       //メッシュの頂点数
        bool bChangeShapeValue;                                 //ウェイトを変更したらtrue

        HumanoidMesh();
    };

    //コンストラクタ
    //基本的にエンジンで実行
    //引数 : std::string FBXファイルのパス, Camera* カメラクラスのポインタ, ID3D12Device* デバイス, ID3D12GraphicsCommandList* コマンドリスト, Camera* カメラ, 
    //          DirectionalLight* ディレクショナルライト, ID3D12Resource* 影が書き込まれるバッファ
    Character(const std::string modelFile, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const Camera* pCamera, const DirectionalLight* pDirectionalLight, 
        MaterialManager* pMaterialManager, float* pFrameTime);
    virtual ~Character();

    //アニメーションを追加
    //戻り値 : 追加されたアニメーションのインデックス
    UINT AddAnimation(Animation* pAnimation);

    //アニメーションを更新
    virtual void Update();

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
            if (mesh.pMesh->meshName != meshName)
                continue;

            return mesh.shapeWeights[shapeIndex];
        }

        return 0.0f;
    }

    //メッシュ情報をすべて取得
    inline std::vector<HumanoidMesh>& GetHumanMeshes() { return m_humanoidMeshes; };

    //メッシュ名からメッシュ情報を取得
	inline HumanoidMesh* GetHumanMesh(std::string meshName)
	{
		for (HumanoidMesh& mesh : m_humanoidMeshes) {
			if (mesh.pMesh->meshName == meshName) {
				return &mesh;
			}
		}
		return nullptr;
	}

    //現在のアニメーションの長さを取得
    inline float GetAnimationLength() const
    {
        if (m_characterAnimations.size() > m_nowAnimationIndex) {
            return 0.0f;
        }

        size_t frameCount = m_characterAnimations[m_nowAnimationIndex].pAnimation->m_frames.size();
        return m_characterAnimations[m_nowAnimationIndex].pAnimation->m_frames[frameCount - 1].time;
    }

    //ボーンマネージャーを取得
    inline BoneManager* GetBoneManager() { return &m_boneManager; }

protected:
    //フレームの最後に実行 (エンジンから呼ばれる)
    virtual void LateUpdate(UINT backBufferIndex);

private:
    struct HumanoidList
    {
        UINT humanoidMeshCount;
        int refCount;

        std::vector<std::unordered_map<std::string, UINT>> shapeMappings;
        std::vector<std::string> meshNames;
        std::vector<UINT> shapeCounts;

        HumanoidList()
            : humanoidMeshCount(0)
            , refCount(1)
        {
        }

        void Release()
        {
            refCount--;
            if (refCount <= 0) {
                shapeMappings.clear();
            }
        }
    };
    static std::unordered_map<std::string, HumanoidList> s_sharedHumanoidMeshes;    //各モデルのメッシュを保持

    //シェーダーに渡す頂点情報
    struct Vertex {
        XMFLOAT3 position;      //頂点の位置 (Tポーズ)
        XMFLOAT4 boneWeights;   //ボーンの影響度
        UINT boneIDs[4];        //4つのボーンから影響を受ける
        UINT vertexID;          //頂点ID
        XMFLOAT3 normal;        //法線
        XMFLOAT2 texCoords;     //UV
		XMFLOAT3 tangent;       //接線
		XMFLOAT3 bitangent;     //従法線
    };

    //ボーンが存在するFBXファイルをロード
    void LoadFBX(const std::string& fbxFile);
	//ボーンが存在するHCSファイルをロード
    void LoadHCS(const std::string& hcsFile);

    //ノードを読み込み
    void ProcessNode(const aiScene* pScene, const aiNode* pNode);
    //メッシュ情報を.fbxファイルから取得
    Mesh* ProcessMesh(aiMesh* mesh, HumanoidMesh& humanoidMesh, UINT meshIndex);
    //メッシュ情報を.hcsファイルから取得
    void ProcessMesh(BinaryReader& br, HumanoidMesh& humanoidMesh, UINT meshIndex);

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
    void SetTexture(Mesh* pMesh, const std::string nameOnly);

    //シェイプキーのウェイトを更新
    void UpdateShapeKeys();
    //アニメーションを更新
    void UpdateAnimation();

    //必要なバッファを作成
    void CreateBuffer(HumanoidMesh& humanoidMesh);
    //ヒューマノイドに必要なバッファを作成
    void CreateHumanoidMeshBuffer(HumanoidMesh& humanoidMesh);
    //シェイプキーのデータをテクスチャとして作成 (テクスチャにすることで処理速度大幅UP)
    void CreateShapeDeltasTexture(bool bClearDeltas);

private:  //プライベート変数
    struct CharacterAnimation
    {
        Animation* pAnimation;

        //1フレーム前のアニメーションフレームのインデックス
        UINT beforeFrameIndex;
    };

    std::vector<HumanoidMesh> m_humanoidMeshes;             //ヒューマノイド用のメッシュ情報
    std::vector<CharacterAnimation> m_characterAnimations;  //アニメーション情報
    std::vector<ID3D12Resource*> shapeBuffers;

    BoneManager m_boneManager;      //ボーンに関する情報を保持する構造体

    bool m_bHCSFile;
};