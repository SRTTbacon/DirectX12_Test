#pragma once
#include <d3dx12.h>
#include <DirectXMath.h>
#include "..\..\\..\\ComPtr.h"

constexpr int MAX_BONES = 4;

struct alignas(256) Transform
{
    DirectX::XMMATRIX World; // ワールド行列
    DirectX::XMMATRIX View; // ビュー行列
    DirectX::XMMATRIX Proj; // 投影行列
    DirectX::XMMATRIX BoneTransforms[MAX_BONES]; // ボーントランスフォーム行列
};
