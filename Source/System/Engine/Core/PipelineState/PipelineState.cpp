#include "PipelineState.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

PipelineState::PipelineState(ID3D12Device* device, ID3D12RootSignature* rootSignature) {
    // �p�C�v���C���X�e�[�g�I�u�W�F�N�g�̍쐬
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

    // ���̓��C�A�E�g�̒�`
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BONEIDS", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };

    if (!rootSignature) {
        printf("RootSignature�������ł��B\n");
        return; // �G���[�n���h�����O
    }

    psoDesc.pRootSignature = rootSignature;

    // �V�F�[�_�[�o�C�i����ݒ�
    ComPtr<ID3DBlob> pVsBlob, pPsBlob;
    HRESULT hr = D3DReadFileToBlob(L"x64/Debug/SimpleVS.cso", pVsBlob.GetAddressOf());
    if (FAILED(hr) || !pVsBlob)
    {
        printf("���_�V�F�[�_�[�̓ǂݍ��݂Ɏ��s�B�G���[�R�[�h:%lx\n", hr);
        return;
    }
    hr = D3DReadFileToBlob(L"x64/Debug/SimplePS.cso", pPsBlob.GetAddressOf());
    if (FAILED(hr) || !pPsBlob)
    {
        printf("�s�N�Z���V�F�[�_�[�̓ǂݍ��݂Ɏ��s�B�G���[�R�[�h:%lx\n", hr);
        return;
    }

    psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVsBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPsBlob.Get());

    // ���X�^���C�U�[�X�e�[�g�i�f�t�H���g�j
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // �J�����O�͂Ȃ�

    // �u�����h�X�e�[�g�i�f�t�H���g�j
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    // �[�x�E�X�e���V���X�e�[�g�i�f�t�H���g�j
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // �����_�[�^�[�Q�b�g�̐ݒ�i1�̃����_�[�^�[�Q�b�g�ARGBA8�t�H�[�}�b�g�j
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    // �}���`�T���v�����O�̐ݒ�
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    // �p�C�v���C���X�e�[�g�̍쐬
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
    if (FAILED(hr)) {
        printf("�p�C�v���C���X�e�[�g�̍쐬�Ɏ��s���܂����B�G���[�R�[�h:%lx\n", hr);
    }
}

ID3D12PipelineState* PipelineState::GetPipelineState() const {
    return pipelineState.Get();
}
