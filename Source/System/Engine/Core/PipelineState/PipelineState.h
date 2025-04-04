#pragma once
#include "..\\..\\..\\..\\KeyString.h"
#include "..\\RootSignature\\RootSignature.h"

class PipelineState {
public:
    PipelineState() = default;
    //�p�C�v���C���X�e�[�g���쐬 (���f�����ɕK�v)
    PipelineState(ID3D12Device* pDevice, RootSignature* pRootSignature, D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_NONE);

    ID3D12PipelineState* GetPipelineState() const;

private:
    ComPtr<ID3D12PipelineState> pipelineState;

    UINT m_elementCount;

    void SetAlphaBlend(D3D12_BLEND_DESC& desc);

    D3D12_INPUT_ELEMENT_DESC* GetInputElement(ShaderKinds shaderKind);
};
