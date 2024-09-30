#pragma once
#include <d3dx12.h>
#include <DirectXMath.h>
#include "..\..\\..\\ComPtr.h"

constexpr int MAX_BONES = 4;

struct Vertex
{
    DirectX::XMFLOAT3 Position; // �ʒu���W
    DirectX::XMFLOAT3 Normal; // �@��
    DirectX::XMFLOAT2 UV; // uv���W
    DirectX::XMFLOAT3 Tangent; // �ڋ��
    //DirectX::XMFLOAT4 Color; // ���_�F
    uint32_t boneIndices[4];       // �{�[���C���f�b�N�X�i�ő�4�̃{�[���j
    float weights[4];              // �{�[���E�F�C�g�i�Ή�����E�F�C�g�j


    static const D3D12_INPUT_LAYOUT_DESC InputLayout;

private:
    static const int InputElementCount = 5;
    static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
};

struct alignas(256) Transform
{
    DirectX::XMMATRIX World; // ���[���h�s��
    DirectX::XMMATRIX View; // �r���[�s��
    DirectX::XMMATRIX Proj; // ���e�s��
    DirectX::XMMATRIX BoneTransforms[MAX_BONES]; // �{�[���g�����X�t�H�[���s��
};

struct Mesh
{
    std::vector<Vertex> Vertices; // ���_�f�[�^�̔z��
    std::vector<uint32_t> Indices; // �C���f�b�N�X�̔z��
    std::wstring DiffuseMap; // �e�N�X�`���̃t�@�C���p�X
    const char* MeshName;  //���b�V����
};
