#pragma once

#include <unordered_map>
#include <fstream>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "..\\Core\\ResourceCopy\\ResourceCopy.h"
#include "..\\Camera\\Camera.h"
#include "Animation\\Animation.h"
#include "Bone\\Bone.h"
#include "Material\\MaterialManager.h"
#include "Transform\\Transform.h"
#include "Mesh.h"

using namespace DirectX;

//モデルファイルのヘッダー
constexpr const char* MODEL_HEADER = "HCSModel";
//モデルファイルがキャラクターの場合は0
constexpr BYTE MODEL_CHARACTER = 0;
constexpr BYTE MODEL_DEFAULT = 1;

class Model;

enum ModelType
{
    ModelType_Unknown,      //未指定
    ModelType_Primitive,    //普通のモデル
    ModelType_Character,    //ボーンが存在するモデル
};

//メッシュごとに必要な情報
struct Mesh : public Transform {

    //メッシュのマトリックス情報
    struct MeshCostantBuffer
    {
        XMMATRIX meshMatrix;    //メッシュの位置、角度、スケール
        XMMATRIX normalMatrix;  //メッシュとモデル全体のマトリックスを掛けたもの
        float time;             //時間情報（シェーダーで動的に更新）
    };

    std::shared_ptr<MeshData> meshData;             //共有メッシュデータ

    ComPtr<ID3D12Resource> shapeWeightsBuffer;      //シェイプキーのウェイト情報              (ヒューマノイドモデルのみ設定)
    void* pShapeWeightsMapped;                      //シェイプキーのウェイト情報のMap         (ヒューマノイドモデルのみ設定)

    Material* pMaterial;                            //マテリアル
    Model* pModel;                                  //親であるモデルクラス

    std::string meshName;                           //メッシュ名

    bool bDraw;                                     //描画するかどうか
    bool bDrawShadow;                               //影を描画するかどうか

    Mesh();
    ~Mesh();

    //メッシュを描画
    //ModelManagerから実行される
    void DrawMesh(ID3D12GraphicsCommandList* pCommandList, UINT backBufferIndex, XMMATRIX& modelMatrix, bool bShadowMode = false);
};

class Model : public Transform
{
    friend class ModelManager;

public:
    //モデルを初期化
    Model(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const Camera* pCamera, const DirectionalLight* pDirectionalLight, MaterialManager* pMaterialManager, float* pFrameTime);
    virtual ~Model();

    //FBXファイルからモデルを作成
    void LoadModel(const std::string modelFile);

    virtual void Update();

public: //ゲッター関数
    //メッシュ数
    inline UINT GetMeshCount() const { return static_cast<UINT>(m_meshes.size()); }

    //メッシュを取得
    inline Mesh* GetMesh(UINT index) { return m_meshes[index]; }

    //モデル情報が入っているバッファを取得
    inline ID3D12Resource* GetConstantBuffer(UINT backBufferIndex) const { return m_modelConstantBuffer[backBufferIndex].Get(); }

    //ボーンバッファを取得
    inline ID3D12Resource* GetBoneBuffer() const { return m_boneMatricesBuffer.Get(); }

    //モデルタイプを取得
    inline ModelType GetModelType() const { return m_modelType; }

    //深度を取得
    inline float GetZBuffer() const { return m_depth; }

    //既に影のレンダリングが終わっているか
    inline bool GetIsShadowRendered() const { return m_bShadowRendered; }

    //モデルのファイル名
    inline std::string GetModelFilePath() const { return m_modelFile; }

    //モデルが存在している時間を取得
    inline float GetModelTime() const { return m_modelTime; }

public:
    float m_animationSpeed;     //アニメーション速度。1.0fが通常速度
    float m_nowAnimationTime;   //現在のアニメーション時間

    bool m_bDrawShadow;

protected:
    //シェーダーに渡す頂点情報 (プリミティブ用)
    struct VertexPrimitive {
        XMFLOAT3 position;
        XMFLOAT4 boneWeights;   //影用のシェーダーと合わせる必要があるためダミー
        UINT boneIDs[4];        //影用のシェーダーと合わせる必要があるためダミー
        UINT vertexID;          //頂点ID (Index)
        XMFLOAT3 normal;
        XMFLOAT2 texCoords;
		XMFLOAT3 tangent;
		XMFLOAT3 bitangent;
    };

    //シェーダーに渡す頂点数の情報
    struct Contents {
        UINT vertexCount;
        UINT shapeCount;
    };

    //メッシュを共有するための構造体
    struct MeshDataList
    {
        struct MeshDataMain
        {
            std::string meshName;
            DirectX::XMFLOAT3 position;
            DirectX::XMFLOAT3 rotation;
        };

        int refCount;
        std::vector<std::shared_ptr<MeshData>> meshDataList;
        std::vector<MeshDataMain> meshMain;
        std::vector<Animation> animations;

        MeshDataList()
            : refCount(1)
        {
        }

        void Release()
        {
            refCount--;
            if (refCount <= 0) {
                meshDataList.clear();
            }
        }
    };
    static std::unordered_map<std::string, MeshDataList> s_sharedMeshes;    //各モデルのメッシュを保持

    ID3D12Device* m_pDevice;                                            //エンジンのデバイス
    ID3D12GraphicsCommandList* m_pCommandList;                          //エンジンのコマンドリスト
    ComPtr<ID3D12Resource> m_modelConstantBuffer[FRAME_BUFFER_COUNT];   //コンスタントバッファ。画面のちらつきを防止するためトリプルバッファリング (2個でも十分なのかな?)
    void* m_pMappedConstantBuffer[FRAME_BUFFER_COUNT];
    ComPtr<ID3D12Resource> m_boneMatricesBuffer;                        //ボーン情報をシェーダーに送信する用
    void* m_pBoneMatricesMap;                                           //ボーン情報のポインタ

    std::vector<Mesh*> m_meshes;    //メッシュの配列

    const Camera* m_pCamera;                        //カメラ情報
    const DirectionalLight* m_pDirectionalLight;    //ディレクショナルライト情報
    MaterialManager* m_pMaterialManager;            //マテリアル作成&取得

    ModelType m_modelType;
    std::string m_modelFile;

    const float* m_pFrameTime;

    int m_nowAnimationIndex;        //現在再生中のアニメーション番号

    float m_depth;                  //Zバッファ

protected:
    void CreateConstantBuffer();

    template <typename VertexType>
    void CreateBuffer(Mesh* pMesh, std::vector<VertexType>& vertices, std::vector<UINT>& indices, UINT vertexStructSize, UINT meshIndex);

    //更新 (エンジンから実行されるため、ユーザーが実行する必要はない)
    virtual void LateUpdate(UINT backBufferIndex);

private:
    //シェーダーに渡すビュー情報
    struct ModelConstantBuffer {
        XMMATRIX modelMatrix;
        XMMATRIX viewMatrix;
        XMMATRIX projectionMatrix;
        XMMATRIX lightViewProjMatrix;
        XMFLOAT4 cameraPos;
        XMFLOAT4 lightPos;
    };

    std::vector<Animation> m_animations;

    ModelConstantBuffer m_modelConstantStruct;  //頂点シェーダーに送信するメッシュ変数

    float m_modelTime;          //モデルが存在している時間 (秒)
    bool m_bShadowRendered;     //影のレンダリングが終わっているかどうか

private:
    void ProcessNode(const aiScene* scene, aiNode* node);   //ノードを読み込み
    Mesh* ProcessMesh(const aiScene* scene, aiMesh* mesh, UINT meshIndex);  //メッシュ情報を読み込み
    Mesh* ProcessMesh(BinaryReader& br, UINT meshIndex);
    void ProcessAnimation(const aiScene* pScene);
    void ProcessAnimation(BinaryReader& br);

    void UpdateAnimation();

    bool SetSharedMeshes();

    XMMATRIX GetMeshDefaultMatrix(aiNode* pNode);

    //シャドウマップに描画 (エンジンから実行されるため、ユーザーが実行する必要はない)
    void RenderShadowMap(UINT backBufferIndex);
};

//必要なバッファを作成 (キャラクターと普通のモデル両方をに対応)
template <typename VertexType>
void Model::CreateBuffer(Mesh* pMesh, std::vector<VertexType>& vertices, std::vector<UINT>& indices, UINT vertexStructSize, UINT meshIndex)
{
    if (s_sharedMeshes.find(m_modelFile) != s_sharedMeshes.end()) {
        if (meshIndex == 0) {
            s_sharedMeshes[m_modelFile].refCount++;
        }
        if (s_sharedMeshes[m_modelFile].meshDataList.size() > meshIndex) {
            size_t size = s_sharedMeshes[m_modelFile].meshDataList.size();
            pMesh->meshData = s_sharedMeshes[m_modelFile].meshDataList[meshIndex];
            pMesh->meshName = s_sharedMeshes[m_modelFile].meshMain[meshIndex].meshName;
            pMesh->m_position = s_sharedMeshes[m_modelFile].meshMain[meshIndex].position;
            pMesh->m_rotation = s_sharedMeshes[m_modelFile].meshMain[meshIndex].rotation;
            return;
        }
    }
    else {
        s_sharedMeshes[m_modelFile] = MeshDataList();
        s_sharedMeshes[m_modelFile].meshDataList = std::vector<std::shared_ptr<MeshData>>();
        s_sharedMeshes[m_modelFile].meshMain= std::vector<MeshDataList::MeshDataMain>();
        if (meshIndex == 0) {
            s_sharedMeshes[m_modelFile].refCount = 1;
        }
    }

    const UINT bufferSize = static_cast<UINT>(vertexStructSize * vertices.size());

    std::shared_ptr<MeshData> p1(new MeshData());
    s_sharedMeshes[m_modelFile].meshDataList.push_back(p1);
    MeshDataList::MeshDataMain dataMain{};
    s_sharedMeshes[m_modelFile].meshMain.push_back(dataMain);

    pMesh->meshData = p1;
    pMesh->meshData->vertexCount = static_cast<UINT>(vertices.size());

    //ヒープ設定
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    //頂点バッファのリソース
    CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    //デバイスで作成
    HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pMesh->meshData->vertexBuffer));

    if (FAILED(hr)) {
        printf("頂点バッファの生成に失敗しました。%1xl\n", hr);
    }

    ID3D12Resource* pUploadBuffer = g_resourceCopy->CreateUploadBuffer(bufferSize);

    //リソースのコピーは非同期で行う
    std::thread([=] {
        //頂点データをGPUに送信
        void* vertexDataBegin;
        pUploadBuffer->Map(0, nullptr, &vertexDataBegin);
        memcpy(vertexDataBegin, vertices.data(), bufferSize);
        pUploadBuffer->Unmap(0, nullptr);

        g_resourceCopy->BeginCopyResource();
        g_resourceCopy->GetCommandList()->CopyBufferRegion(pMesh->meshData->vertexBuffer.Get(), 0, pUploadBuffer, 0, bufferSize);
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pMesh->meshData->vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        g_resourceCopy->GetCommandList()->ResourceBarrier(1, &barrier);
        g_resourceCopy->EndCopyResource();
        pUploadBuffer->Release();
        }).detach();

    pMesh->meshData->vertexBufferView.BufferLocation = pMesh->meshData->vertexBuffer->GetGPUVirtualAddress();
    pMesh->meshData->vertexBufferView.StrideInBytes = vertexStructSize;
    pMesh->meshData->vertexBufferView.SizeInBytes = bufferSize;

    //インデックスバッファの作成
    const UINT indexBufferSize = static_cast<UINT>(sizeof(UINT) * indices.size());

    //インデックスバッファのリソース
    CD3DX12_RESOURCE_DESC indexBuffer = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

    hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pMesh->meshData->indexBuffer));

    if (FAILED(hr)) {
        printf("インデックスバッファの生成に失敗しました。\n");
    }

    pUploadBuffer = g_resourceCopy->CreateUploadBuffer(indexBufferSize);

    //リソースのコピーは非同期で行う
    std::thread([=] {
        //インデックスデータをGPUに送信
        void* indexDataBegin;
        pUploadBuffer->Map(0, nullptr, &indexDataBegin);
        memcpy(indexDataBegin, indices.data(), indexBufferSize);
        pUploadBuffer->Unmap(0, nullptr);

        g_resourceCopy->BeginCopyResource();
        g_resourceCopy->GetCommandList()->CopyBufferRegion(pMesh->meshData->indexBuffer.Get(), 0, pUploadBuffer, 0, indexBufferSize);
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pMesh->meshData->indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        g_resourceCopy->GetCommandList()->ResourceBarrier(1, &barrier);
        g_resourceCopy->EndCopyResource();
        pUploadBuffer->Release();
        }).detach();

    pMesh->meshData->indexBufferView.BufferLocation = pMesh->meshData->indexBuffer->GetGPUVirtualAddress();
    pMesh->meshData->indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    pMesh->meshData->indexBufferView.SizeInBytes = indexBufferSize;

    pMesh->meshData->indexCount = static_cast<UINT>(indices.size());

    //頂点数を保持する用のリソースを作成
    CD3DX12_RESOURCE_DESC contentsBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Contents));
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &contentsBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pMesh->meshData->contentsBuffer));
    if (FAILED(hr)) {
        printf("コンテンツバッファの生成に失敗しました。\n");
    }

    //頂点数とシェイプキー数を保持
    Contents contents{};
    contents.vertexCount = 1;
    contents.shapeCount = 0;
    void* pContentsBuffer;
    pMesh->meshData->contentsBuffer->Map(0, nullptr, &pContentsBuffer);
    if (pContentsBuffer)
        memcpy(pContentsBuffer, &contents, sizeof(Contents));
    pMesh->meshData->contentsBuffer->Unmap(0, nullptr);
}
