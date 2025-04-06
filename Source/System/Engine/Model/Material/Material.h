#pragma once

#include "..\\..\\Core\\RootSignature\\RootSignature.h"
#include "..\\..\\Core\\PipelineState\\PipelineState.h"
#include "..\\..\\Core\\DescriptorHeap\\DescriptorHeap2.h"
#include "..\\..\\Lights\\DirectionalLight.h"
#include "..\\..\\Core\\Texture2D\\Texture2D.h"

class Material
{
    friend class ModelManager;

public:
    Material(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, DirectionalLight* pDirectionalLight, DescriptorHeap* pDescriptorHeap, UINT materialID);
    ~Material();

    //デフォルトのテクスチャをセット
    void Initialize(ID3D12Resource* pDefaultMainTex, ID3D12Resource* pDefaultNormalTex);

    //ルートシグネチャとパイプラインステートを指定
    void SetPipeline(RootSignature* pRootSignature, PipelineState* pPipelineState, ShaderKinds shaderKind);
    //テクスチャ張り替え
    void SetMainTexture(std::string texPath);
    void SetMainTexture(ID3D12Resource* pResource);
    //ノーマルマップ張り替え
    void SetNormalMap(std::string texPath);
    //シェイプキーのリソースを設定
    //引数 : Meshクラス内のshapeDataIndex, ID3D12Resource* シェイプ情報のリソース
    void SetShapeData(UINT index, ID3D12Resource* pShapeTexture);

    void SetIsTransparent(bool bTransparent);
    void SetOpacity(float value);

public: //ゲッター
    //シェイプ情報が入っているハンドルを取得
    //引数 : Meshクラス内のshapeDataIndex
    D3D12_GPU_DESCRIPTOR_HANDLE GetShapeData(UINT offset);

    //子のマテリアルが半透明かどうかを取得
    bool GetIsTransparent() const;

    float GetOpacity() const { return m_transparentValue; }

public:
    DirectX::XMFLOAT4 m_ambientColor;
    DirectX::XMFLOAT4 m_diffuseColor;

private:
    ID3D12Device* m_pDevice;                    //エンジンのデバイス
    ID3D12GraphicsCommandList* m_pCommandList;  //エンジンのコマンドリスト

    RootSignature* m_pRootSignature;            //ルートシグネチャ
    PipelineState* m_pPipelineState;            //パイプラインステート
    DescriptorHeap* m_pDescriptorHeap;          //マテリアル

    DirectionalLight* m_pDirectionalLight;      //環境光
    ComPtr<ID3D12Resource> m_psBufferResource;

    const UINT m_materialID;                    //マテリアルID (ディスクリプタヒープのIndex。MaterialManagerで管理)

    Texture2D* m_pMainTexture;      //テクスチャ
    Texture2D* m_pNormalTexture;    //ノーマルマップ

    ShaderKinds m_shaderKind;       //シェーダーの種類

    float m_transparentValue;
    bool m_bTransparent;    //半透明のオブジェクトかどうか

    void* m_pPSBufferMap;

    //ルートシグネチャとパイプラインステートをGPUに送信
    void ExecutePipeline();
    //テクスチャ情報をGPUに送信
    void ExecuteShapeData(UINT index);

    //コピーガード
    void operator =(const Material& src){};
    Material(const Material& src);
};