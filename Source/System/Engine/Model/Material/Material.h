#pragma once

#include "..\\..\\Core\\RootSignature\\RootSignature.h"
#include "..\\..\\Core\\PipelineState\\PipelineState.h"
#include "..\\..\\Core\\DescriptorHeap\\DescriptorHeap2.h"
#include "..\\..\\Lights\\DirectionalLight.h"
#include "..\\..\\Core\\Texture2D\\Texture2D.h"

class Material
{
public:
    Material(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, DirectionalLight* pDirectionalLight, DescriptorHeap* pDescriptorHeap, UINT materialID);
    ~Material();

    //デフォルトのテクスチャをセット
    void Initialize(ID3D12Resource* pDefaultMainTex, ID3D12Resource* pDefaultNormalTex);

    //ルートシグネチャとパイプラインステートを指定
    void SetPipeline(RootSignature* pRootSignature, PipelineState* pPipelineState, ShaderKinds shaderKind);
    //テクスチャ張り替え
    void SetMainTexture(std::string texPath);
    //ノーマルマップ張り替え
    void SetNormalMap(std::string texPath);
    //シェイプキーのリソースを設定
    void SetShapeData(UINT index, ID3D12Resource* pShapeTexture);

    void SetIsTransparent(bool bTransparent);

    //ルートシグネチャとパイプラインステートをGPUに送信
    void ExecutePipeline();
    //テクスチャ情報をGPUに送信
    void ExecuteShapeData(UINT index);

public: //ゲッター
    D3D12_GPU_DESCRIPTOR_HANDLE GetShapeData(UINT offset);
    bool GetIsTransparent() const;

private:
    ID3D12Device* m_pDevice;                    //エンジンのデバイス
    ID3D12GraphicsCommandList* m_pCommandList;  //エンジンのコマンドリスト

    RootSignature* m_pRootSignature;            //ルートシグネチャ
    PipelineState* m_pPipelineState;            //パイプラインステート
    DescriptorHeap* m_pDescriptorHeap;          //マテリアル

    DirectionalLight* m_pDirectionalLight;      //環境光

    const UINT m_materialID;                 //マテリアルID (ディスクリプタヒープのIndex。MaterialManagerで管理)

    Texture2D* m_pMainTexture;      //テクスチャ
    Texture2D* m_pNormalTexture;    //ノーマルマップ

    ShaderKinds m_shaderKind;       //シェーダーの種類

    bool m_bTransparent;    //半透明のオブジェクトかどうか

    //コピーガード
    void operator =(const Material& src){};
    Material(const Material& src);
};