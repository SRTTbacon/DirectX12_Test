#pragma once
#include <d3d12.h>
#include <d3dx12.h>
#include "..\\..\\..\\ComPtr.h"
#include "..\\DescriptorHeap\\DescriptorHeap2.h"

enum ShaderKinds
{
    BoneShader,
    PrimitiveShader,
    ShadowShader
};

class RootSignature {
public:

    RootSignature(ID3D12Device* device, ShaderKinds shaderKind);
    ID3D12RootSignature* GetRootSignature() const;

    ShaderKinds m_shaderKind;

private:
    ComPtr<ID3D12RootSignature> rootSignature;

    CD3DX12_DESCRIPTOR_RANGE* m_pTableRange1;
    CD3DX12_DESCRIPTOR_RANGE* m_pTableRange2;
    UINT m_rootParamSize;

    CD3DX12_ROOT_PARAMETER* GetRootParameter();
};