#pragma once
#include <d3d12.h>
#include <d3dx12.h>
#include "..\\..\\..\\ComPtr.h"
#include "..\\DescriptorHeap\\DescriptorHeap2.h"

enum ShaderKinds
{
    UnknownShader,
    BoneShader,         //ボーンが存在するモデル
    PrimitiveShader,    //単純なテクスチャのみのモデル
    TerrainShader,      //地形用モデル
    TerrainShaderFront, //地形用モデル
    GrassShader,        //草
    ShadowShader,       //影用
    SkyBoxShader,       //スカイボックス用
    UITextureShader,    //UI用
    PostProcessShader,  //ポストプロセス用
};

class RootSignature {
public:

    RootSignature();
    RootSignature(ID3D12Device* pDevice, ShaderKinds shaderKind);
    ID3D12RootSignature* GetRootSignature() const;

public:
    inline ShaderKinds GetShaderKind() const { return m_shaderKind; }

private:
    ComPtr<ID3D12RootSignature> m_rootSignature;

    CD3DX12_DESCRIPTOR_RANGE* m_pDiscriptorRange;

    UINT m_rootParamSize;

    CD3DX12_ROOT_PARAMETER* GetRootParameter();

    ShaderKinds m_shaderKind;
};