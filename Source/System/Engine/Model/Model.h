#pragma once
#include <assimp/scene.h>
#include <string>
#include <unordered_map>
#include "..\\Core\\RootSignature\\RootSignature.h"
#include "..\\Core\\PipelineState\\PipelineState.h"
#include "..\\Core\\DescriptorHeap\\DescriptorHeap.h"
#include "..\\Core\\Texture2D\\Texture2D.h"
#include "..\\Camera\\Camera.h"

using namespace DirectX;

constexpr int MAX_BONE_COUNT = 300;

struct Vertex {
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT2 TexCoords;
    XMFLOAT4 Color; // 頂点色
    XMFLOAT4 BoneWeights;
    UINT BoneIDs[4];
};

struct ModelConstantBuffer {
    XMMATRIX modelMatrix;
    XMMATRIX viewMatrix;
    XMMATRIX projectionMatrix;
};

class Model
{
public:
    Model(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::string& fbxFile, const Camera* pCamera);

    void Update();
    void Draw(ID3D12GraphicsCommandList* commandList);

    XMFLOAT3 m_position;    //モデル全体の位置
    XMFLOAT3 m_rotation;    //モデル全体の回転 (デグリー角)
    XMFLOAT3 m_scale;       //モデル全体のスケール

private:
    struct BoneNode {
        std::string boneName;           //ボーン名
        std::vector<UINT> childBones;   //子ボーン
        XMMATRIX boneOffset;
        UINT parentBoneIndex;           //親ボーンのインデックス
        XMFLOAT3 m_position;            //ボーンの位置
        XMFLOAT3 m_rotation;            //ボーンの回転 (デグリー角)
        XMFLOAT3 m_scale;               //ボーンのスケール
    };

    struct Mesh {
        ComPtr<ID3D12Resource> vertexBuffer;            //頂点バッファ(GPUメモリに頂点情報を保存)
        ComPtr<ID3D12Resource> indexBuffer;             //インデックスバッファ(GPUメモリに入っている頂点バッファの各頂点にインデックスを付けて保存)
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView;      //頂点バッファのデータ内容とサイズを保持
        D3D12_INDEX_BUFFER_VIEW indexBufferView;        //インデックスバッファのデータ内容とサイズを保持
        UINT indexCount;                                //インデックス数 (GPU側で、この数ぶん描画させる)
        BYTE materialIndex;                             //マテリアルが入っているインデックス (同じテクスチャは使いまわす)
    };

    ID3D12Device* device;                                           //エンジンのデバイス
    RootSignature* m_pRootSignature;                                //ルートシグネチャ
    PipelineState* m_pPipelineState;                                //パイプラインステート
    DescriptorHeap* descriptorHeap;                                 //マテリアル
    ComPtr<ID3D12Resource> modelConstantBuffer[FRAME_BUFFER_COUNT]; //画面のちらつきを防止するためトリプルバッファリング
    ComPtr<ID3D12Resource> m_boneMatricesBuffer;

    const Camera* m_pCamera;

    std::vector<Mesh> meshes;                       //メッシュの配列 (キャラクターなど、FBX内に複数のメッシュが存在するものに対応)
    std::vector<XMMATRIX> boneInfos;
    std::vector<XMMATRIX> finalBoneInfos;
    std::vector<BoneNode> boneWorlds;
    std::unordered_map<std::string, UINT> boneMapping;

    XMMATRIX m_modelMatrix;                         //位置、回転、スケールをMatrixで保持

    void LoadFBX(const std::string& fbxFile);
    void ProcessNode(const aiScene* scene, aiNode* node);
    Mesh ProcessMesh(const aiScene* scene, aiMesh* mesh);
    void LoadBones(const aiScene* scene, Mesh& meshStruct, aiMesh* mesh, std::vector<Vertex>& vertices);
    void LoadBoneFamily(const aiNode* node);
    void UpdateBoneTransform(std::string boneName, XMFLOAT3& position);
    void UpdateBoneTransform(UINT boneIndex, XMMATRIX& parentMatrix);
    void UpdateBoneTransform();
};