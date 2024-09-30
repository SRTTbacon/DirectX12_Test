#pragma once
#include <d3d12.h>
#include "..\\..\\..\\ComPtr.h"

class RootSignature {
public:
    RootSignature(ID3D12Device* device);
    ID3D12RootSignature* GetRootSignature() const;

private:
    ComPtr<ID3D12RootSignature> rootSignature;
};