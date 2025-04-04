#pragma once
#include <d3d12.h>
#include <d3dx12.h>
#include "..\\..\\..\\ComPtr.h"
#include "..\\DescriptorHeap\\DescriptorHeap2.h"

enum ShaderKinds
{
    UnknownShader,
    BoneShader,         //�{�[�������݂��郂�f��
    PrimitiveShader,    //�P���ȃe�N�X�`���݂̂̃��f��
    TerrainShader,      //�n�`�p���f��
    TerrainShaderFront, //�n�`�p���f��
    GrassShader,        //��
    ShadowShader,       //�e�p
    SkyBoxShader,       //�X�J�C�{�b�N�X�p
    UITextureShader,    //UI�p
    PostProcessShader,  //�|�X�g�v���Z�X�p
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