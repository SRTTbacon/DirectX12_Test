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

//モデルファイルのヘッダー
constexpr const char* MODEL_HEADER = "HCSModel";
//モデルファイルがキャラクターの場合は0
constexpr BYTE MODEL_CHARACTER = 0;

enum ModelType
{
    ModelType_Unknown,      //未指定
    ModelType_Primitive,    //普通のモデル
    ModelType_Character,    //ボーンが存在するモデル
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
    std::string meshName;                           //メッシュ名
    UINT vertexCount;                               //頂点数
    UINT indexCount;                                //インデックス数 (GPU側で、この数ぶん描画させる)
	bool bDraw;                                     //描画するかどうか

    Mesh();
    ~Mesh();
};

class Model
{
public:
    //モデルを初期化
    Model(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const Camera* pCamera, DirectionalLight* pDirectionalLight, ID3D12Resource* pShadowMapBuffer);
    ~Model();

    void LoadModel(const std::string fbxFile);

    //更新 (エンジンから実行されるため、ユーザーが実行する必要はない)
    virtual void LateUpdate(UINT backBufferIndex);

    //シャドウマップに描画 (エンジンから実行されるため、ユーザーが実行する必要はない)
    void RenderShadowMap(UINT backBufferIndex);
    //実際に描画 (エンジンから実行されるため、ユーザーが実行する必要はない)
    void RenderSceneWithShadow(UINT backBufferIndex);

    //半透明オブジェクトかどうかを設定
    void SetTransparent(bool bTransparent);

    std::vector<Mesh*> m_meshes;    //メッシュの配列

    XMFLOAT3 m_position;    //モデル全体の位置
    XMFLOAT3 m_rotation;    //モデル全体の回転 (デグリー角)
    XMFLOAT3 m_scale;       //モデル全体のスケール

    bool m_bVisible;        //描画するかどうか

public: //ゲッター関数
    //深度
    inline float GetZBuffer() const { return m_depth; }

    //半透明かどうか
    inline bool GetIsTransparent() const { return m_bTransparent; }

    //メッシュ内のテクスチャを参照
    inline Texture2D* GetTexture(int index) const { return m_textures[index]; }

    inline std::string GetModelFilePath() const { return m_modelFile; }

protected:
    //シェーダーに渡す頂点情報 (プリミティブ用)
    struct VertexPrimitive {
        XMFLOAT3 position;
        XMFLOAT4 boneWeights;   //影用のシェーダーと合わせる必要があるためダミー
        UINT boneIDs[4];        //影用のシェーダーと合わせる必要があるためダミー
        UINT vertexID;          //頂点ID
        XMFLOAT3 normal;
        XMFLOAT2 texCoords;
		XMFLOAT3 tangent;
		XMFLOAT3 bitangent;
    };

    //シェーダーに渡すビュー情報
    struct ModelConstantBuffer {
        XMMATRIX modelMatrix;
        XMMATRIX viewMatrix;
        XMMATRIX projectionMatrix;
        XMMATRIX lightViewProjMatrix;
        XMMATRIX normalMatrix;
		XMFLOAT4 cameraPos;
        XMFLOAT4 lightPos;
    };

    //シェーダーに渡す頂点数の情報
    struct Contents {
        UINT vertexCount;
        UINT shapeCount;
    };

    ID3D12Device* m_pDevice;                                            //エンジンのデバイス
    ID3D12Resource* m_pShadowMapBuffer;                                 //影のテクスチャ(エンジンから貰う)
    ID3D12GraphicsCommandList* m_pCommandList;                          //エンジンのコマンドリスト
    RootSignature* m_pRootSignature;                                    //ルートシグネチャ
    PipelineState* m_pPipelineState;                                    //パイプラインステート
    DescriptorHeap* m_pDescriptorHeap;                                  //マテリアル
    ComPtr<ID3D12Resource> m_modelConstantBuffer[FRAME_BUFFER_COUNT];   //コンスタントバッファ。画面のちらつきを防止するためトリプルバッファリング (2個でも十分なのかな?)
    ComPtr<ID3D12Resource> m_boneMatricesBuffer;                        //ボーン情報をシェーダーに送信する用
    ComPtr<ID3D12Resource> m_shadowBoneMatricesBuffer;                  //影用シェーダーにボーン情報を送信する用(ボーンがない場合は0で初期化)

    std::vector<Texture2D*> m_textures;                      //テクスチャ情報

    const Camera* m_pCamera;        //カメラ情報
    DirectionalLight* m_pDirectionalLight;    //ディレクショナルライト

    XMMATRIX m_modelMatrix;         //位置、回転、スケールをMatrixで保持

    ModelType m_modelType;
    std::string m_modelFile;

    float m_depth;          //Zバッファ

    void CreateConstantBuffer();

    template <typename VertexType>
    void CreateBuffer(Mesh* pMesh, std::vector<VertexType>& vertices, std::vector<UINT>& indices, UINT vertexStructSize);

private:
    void ProcessNode(const aiScene* scene, aiNode* node);   //ノードを読み込み
    Mesh* ProcessMesh(const aiScene* scene, aiMesh* mesh);  //メッシュ情報を読み込み
    void DrawMesh(const Mesh* pMesh) const;


    bool m_bTransparent;    //半透明のオブジェクトかどうか
};

//必要なバッファを作成 (キャラクターと普通のモデル両方をに対応)
template <typename VertexType>
void Model::CreateBuffer(Mesh* pMesh, std::vector<VertexType>& vertices, std::vector<UINT>& indices, UINT vertexStructSize)
{
    const UINT bufferSize = static_cast<UINT>(vertexStructSize * vertices.size());

    pMesh->vertexCount = static_cast<UINT>(vertices.size());

    //ヒープ設定
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    //頂点バッファのリソース
    CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    //デバイスで作成
    HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pMesh->vertexBuffer));

    if (FAILED(hr)) {
        printf("頂点バッファの生成に失敗しました。%1xl\n", hr);
    }

    //頂点データをGPUに送信
    void* vertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    pMesh->vertexBuffer->Map(0, &readRange, &vertexDataBegin);
    memcpy(vertexDataBegin, vertices.data(), bufferSize);
    pMesh->vertexBuffer->Unmap(0, nullptr);

    pMesh->vertexBufferView.BufferLocation = pMesh->vertexBuffer->GetGPUVirtualAddress();
    pMesh->vertexBufferView.StrideInBytes = vertexStructSize;
    pMesh->vertexBufferView.SizeInBytes = bufferSize;

    //インデックスバッファの作成
    const UINT indexBufferSize = static_cast<UINT>(sizeof(UINT) * indices.size());

    //インデックスバッファのリソース
    CD3DX12_RESOURCE_DESC indexBuffer = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

    hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &indexBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pMesh->indexBuffer));

    if (FAILED(hr)) {
        printf("インデックスバッファの生成に失敗しました。\n");
    }

    //インデックスデータをGPUに送信
    void* indexDataBegin;
    pMesh->indexBuffer->Map(0, &readRange, &indexDataBegin);
    memcpy(indexDataBegin, indices.data(), indexBufferSize);
    pMesh->indexBuffer->Unmap(0, nullptr);

    pMesh->indexBufferView.BufferLocation = pMesh->indexBuffer->GetGPUVirtualAddress();
    pMesh->indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    pMesh->indexBufferView.SizeInBytes = indexBufferSize;

    pMesh->indexCount = static_cast<UINT>(indices.size());

    //頂点数を保持する用のリソースを作成
    CD3DX12_RESOURCE_DESC contentsBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Contents));
    hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &contentsBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pMesh->contentsBuffer));
    if (FAILED(hr)) {
        printf("コンテンツバッファの生成に失敗しました。\n");
    }

    //頂点数とシェイプキー数を保持
    Contents contents{};
    contents.vertexCount = 1;
    contents.shapeCount = 0;
    void* pContentsBuffer;
    pMesh->contentsBuffer->Map(0, nullptr, &pContentsBuffer);
    if (pContentsBuffer)
        memcpy(pContentsBuffer, &contents, sizeof(Contents));
    pMesh->contentsBuffer->Unmap(0, nullptr);
}