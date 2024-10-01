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

//ボーンは最大512個
constexpr int MAX_BONE_COUNT = 512;

//シェーダーに渡す頂点情報
struct Vertex {
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT2 TexCoords;
    XMFLOAT4 Color; // 頂点色
    XMFLOAT4 BoneWeights;
    UINT BoneIDs[4];
};

//シェーダーに渡すビュー情報
struct ModelConstantBuffer {
    XMMATRIX modelMatrix;
    XMMATRIX viewMatrix;
    XMMATRIX projectionMatrix;
};

class Model
{
public:
    Model(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::string& fbxFile, const Camera* pCamera);

    //更新
    void Update();

    //描画
    void Draw(ID3D12GraphicsCommandList* commandList);

    //ボーンの位置を更新
    void UpdateBonePosition(std::string boneName, XMFLOAT3& position);
    //ボーンの回転を変更
    void UpdateBoneRotation(std::string boneName, XMFLOAT3& rotation);
    //ボーンのスケールを変更
    void UpdateBoneScale(std::string boneName, XMFLOAT3& scale);

    XMFLOAT3 m_position;    //モデル全体の位置
    XMFLOAT3 m_rotation;    //モデル全体の回転 (デグリー角)
    XMFLOAT3 m_scale;       //モデル全体のスケール

private:
    struct BoneNode {
        std::string boneName;           //ボーン名
        std::vector<UINT> childBones;   //子ボーン
        XMMATRIX boneOffset;            //ボーンの原点との差
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

    const Camera* m_pCamera;                        //カメラ情報

    std::vector<Mesh> meshes;                           //メッシュの配列 (キャラクターなど、FBX内に複数のメッシュが存在するものに対応)
    std::vector<XMMATRIX> boneInfos;                    //シェーダーに送信するボーンのマトリックス
    std::vector<XMMATRIX> finalBoneInfos;
    std::vector<BoneNode> boneWorlds;                   //ボーン情報
    std::unordered_map<std::string, UINT> boneMapping;  //ボーン名からインデックスを取得

    XMMATRIX m_modelMatrix;         //位置、回転、スケールをMatrixで保持

    void LoadFBX(const std::string& fbxFile);               //FBXをロード
    void ProcessNode(const aiScene* scene, aiNode* node);
    Mesh ProcessMesh(const aiScene* scene, aiMesh* mesh);
    void LoadBones(const aiScene* scene, Mesh& meshStruct, aiMesh* mesh, std::vector<Vertex>& vertices);    //ボーン情報を取得
    void LoadBoneFamily(const aiNode* node);        //ボーンの親子関係を取得
    void UpdateBoneTransform(UINT boneIndex, XMMATRIX& parentMatrix);   //シェーダーに渡すボーンの座標を計算
    void UpdateBoneTransform();                                         //シェーダーに渡すボーンの座標を計算
};