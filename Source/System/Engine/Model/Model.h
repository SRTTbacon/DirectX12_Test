#pragma once
#include <unordered_map>
#include "..\\Core\\RootSignature\\RootSignature.h"
#include "..\\Core\\PipelineState\\PipelineState.h"
#include "..\\Core\\DescriptorHeap\\DescriptorHeap2.h"
#include "..\\Core\\Texture2D\\Texture2D.h"
#include "..\\Camera\\Camera.h"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include "Bone\\Bone.h"
#include "..\\Lights\\DirectionalLight.h"

using namespace DirectX;

//ボーンは最大512個
constexpr int MAX_BONE_COUNT = 512;

class Model
{
public:
    //モデルを初期化
    Model(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const Camera* pCamera, DirectionalLight* pDirectionalLight, ID3D12Resource* pShadowMapBuffer);

    void LoadModel(const std::string fbxFile);

    //更新 (エンジンから実行されるため、ユーザーが実行する必要はない)
    virtual void Update(UINT backBufferIndex);

    //シャドウマップに描画 (エンジンから実行されるため、ユーザーが実行する必要はない)
    void RenderShadowMap(UINT backBufferIndex);
    //実際に描画 (エンジンから実行されるため、ユーザーが実行する必要はない)
    void RenderSceneWithShadow(UINT backBufferIndex);

    XMFLOAT3 m_position;    //モデル全体の位置
    XMFLOAT3 m_rotation;    //モデル全体の回転 (デグリー角)
    XMFLOAT3 m_scale;       //モデル全体のスケール

    bool m_bVisible;        //描画するかどうか

public: //ゲッター関数
    //深度
    inline float GetZBuffer() const
    {
        return m_depth;
    }

    //半透明かどうか
    inline bool GetIsTransparent() const
    {
        return m_bTransparent;
    }

protected:
    //シェーダーに渡す頂点情報 (プリミティブ用)
    struct VertexPrimitive {
        XMFLOAT3 position;
        XMFLOAT4 boneWeights;   //ボーンの影響度
        UINT boneIDs[4];        //4つのボーンから影響を受ける
        XMFLOAT3 normal;
        XMFLOAT2 texCoords;
    };

    //シェーダーに渡すビュー情報
    struct ModelConstantBuffer {
        XMMATRIX modelMatrix;
        XMMATRIX viewMatrix;
        XMMATRIX projectionMatrix;
        XMMATRIX lightViewProjMatrix;
        XMMATRIX normalMatrix;
    };

    //メッシュごとに必要な情報
    struct Mesh {
        ComPtr<ID3D12Resource> vertexBuffer;            //頂点バッファ(GPUメモリに頂点情報を保存)
        ComPtr<ID3D12Resource> indexBuffer;             //インデックスバッファ(GPUメモリに入っている頂点バッファの各頂点にインデックスを付けて保存)
        ComPtr<ID3D12Resource> contentsBuffer;          //頂点数やシェイプキーの数など初期化後一度だけ送信する用 (ヒューマノイドモデルのみ設定)
        ComPtr<ID3D12Resource> shapeWeightsBuffer;      //シェイプキーのウェイト情報                             (ヒューマノイドモデルのみ設定)
        ComPtr<ID3D12Resource> shapeDeltasBuffer;       //各頂点に対するシェイプキーの位置情報                   (ヒューマノイドモデルのみ設定)
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView;      //頂点バッファのデータ内容とサイズを保持
        D3D12_INDEX_BUFFER_VIEW indexBufferView;        //インデックスバッファのデータ内容とサイズを保持
        UINT indexCount;                                //インデックス数 (GPU側で、この数ぶん描画させる)
        //char materialIndex;                             //マテリアルが入っているインデックス (同じテクスチャは使いまわす)

        Mesh();

        void Draw(ID3D12GraphicsCommandList* pCommandList) const;
    };

    ID3D12Device* m_pDevice;                                            //エンジンのデバイス
    ID3D12Resource* m_pShadowMapBuffer;                                 //影のテクスチャ(エンジンから貰う)
    ID3D12GraphicsCommandList* m_pCommandList;                          //エンジンのコマンドリスト
    RootSignature* m_pRootSignature;                                    //ルートシグネチャ
    PipelineState* m_pPipelineState;                                    //パイプラインステート
    DescriptorHeap* m_pDescriptorHeap;                                  //マテリアル
    ComPtr<ID3D12Resource> m_modelConstantBuffer[FRAME_BUFFER_COUNT];   //コンスタントバッファ。画面のちらつきを防止するためトリプルバッファリング (2個でも十分なのかな?)
    ComPtr<ID3D12Resource> m_boneMatricesBuffer;                        //ボーン情報をシェーダーに送信する用
    ComPtr<ID3D12Resource> m_lightConstantBuffer;                       //ディレクショナルライトのバッファ

    const Camera* m_pCamera;        //カメラ情報
    const DirectionalLight* m_pDirectionalLight;    //ディレクショナルライト

    std::vector<Mesh*> m_meshes;    //メッシュの配列 (キャラクターなど、FBX内に複数のメッシュが存在するものに対応)

    XMMATRIX m_modelMatrix;         //位置、回転、スケールをMatrixで保持

private:
    void CreateBuffer(Mesh* pMesh, std::vector<VertexPrimitive>& vertices, std::vector<UINT>& indices);

    void ProcessNode(const aiScene* scene, aiNode* node);   //ノードを読み込み
    Mesh* ProcessMesh(const aiScene* scene, aiMesh* mesh);   //メッシュ情報を読み込み

    void CreateConstantBuffer();

    float m_depth;          //Zバッファ
    bool m_bTransparent;    //半透明のオブジェクトかどうか
};