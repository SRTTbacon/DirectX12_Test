#pragma once
#include <d3dx12.h>
#include <DirectXMath.h>
#include "..\..\\..\\ComPtr.h"

constexpr int MAX_BONES = 4;

struct alignas(256) Transform
{
    DirectX::XMMATRIX World; // ���[���h�s��
    DirectX::XMMATRIX View; // �r���[�s��
    DirectX::XMMATRIX Proj; // ���e�s��
    DirectX::XMMATRIX BoneTransforms[MAX_BONES]; // �{�[���g�����X�t�H�[���s��
};
