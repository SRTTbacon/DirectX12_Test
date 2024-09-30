#pragma once
#include "..\\..\\Engine.h"
#include "..\\..\\..\\ComPtr.h"

class PipelineState {
public:
    PipelineState(ID3D12Device* device, ID3D12RootSignature* rootSignature);
    ID3D12PipelineState* GetPipelineState() const;

private:
    ComPtr<ID3D12PipelineState> pipelineState;
};
