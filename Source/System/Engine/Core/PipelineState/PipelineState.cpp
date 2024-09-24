#include "PipelineState.h"
#include "..\\..\\Engine.h"
#include <d3dx12.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

PipelineState::PipelineState(ID3D12Device* device) {
    m_pDevice = device;
    m_bInited = false;
}

PipelineState::~PipelineState() {
    // �����I�Ƀ��\�[�X���������܂�
}

void PipelineState::SetVS(std::wstring filePath)
{
    vsFilePath = filePath;
}

void PipelineState::SetPS(std::wstring filePath)
{
    psFilePath = filePath;
}

void PipelineState::CreatePipelineState() {
    // �V�F�[�_�[�R���p�C��
    ID3DBlob* vertexShader = nullptr;
    ID3DBlob* pixelShader = nullptr;

    // �p�C�v���C���X�e�[�g�̐ݒ�
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { /* Input Layout ���w�� */ };
    psoDesc.pRootSignature = nullptr; // Root Signature���w��
    //psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
    //psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1; // Render Target �̐�
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // Render Target �̃t�H�[�}�b�g
    psoDesc.SampleDesc.Count = 1; // MSAA �T���v����
    psoDesc.SampleDesc.Quality = 0; // MSAA �̕i��

    // ���_�V�F�[�_�[�ǂݍ���
    auto hr = D3DReadFileToBlob(vsFilePath.c_str(), &vertexShader);
    if (FAILED(hr))
    {
        printf("���_�V�F�[�_�[�̓ǂݍ��݂Ɏ��s");
        return;
    }
    // �s�N�Z���V�F�[�_�[�ǂݍ���
    hr = D3DReadFileToBlob(psFilePath.c_str(), &pixelShader);
    if (FAILED(hr))
    {
        printf("�s�N�Z���V�F�[�_�[�̓ǂݍ��݂Ɏ��s");
        return;
    }

    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);

    // �p�C�v���C���X�e�[�g�I�u�W�F�N�g���쐬
    hr = m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
    if (FAILED(hr)) {
        // �G���[�n���h�����O
        printf("�V�F�[�_�[�̍쐬�Ɏ��s");
        return;
    }

    // �V�F�[�_�[�����
    if (vertexShader) vertexShader->Release();
    if (pixelShader) pixelShader->Release();

    m_bInited = true;
}
