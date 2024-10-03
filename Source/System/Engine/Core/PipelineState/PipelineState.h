#pragma once
#include "..\\..\\..\\..\\GameBase.h"
#include "..\\RootSignature\\RootSignature.h"

class PipelineState {
public:
    //パイプラインステートを作成 (モデル毎に必要)
    PipelineState(ID3D12Device* device, RootSignature* rootSignature);

    ID3D12PipelineState* GetPipelineState() const;

private:
    ComPtr<ID3D12PipelineState> pipelineState;

    UINT m_elementCount;

    D3D12_INPUT_ELEMENT_DESC* GetInputElement(ShaderKinds shaderKind);
};
