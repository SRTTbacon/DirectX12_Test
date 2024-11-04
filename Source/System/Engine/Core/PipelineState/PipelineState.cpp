#include "PipelineState.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

PipelineState::PipelineState(ID3D12Device* device, RootSignature* rootSignature) 
    : m_elementCount(0)
{
    //�p�C�v���C���X�e�[�g�I�u�W�F�N�g�̍쐬
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

    //���̓��C�A�E�g�̒�`
    D3D12_INPUT_ELEMENT_DESC* pInputElements = GetInputElement(rootSignature->m_shaderKind);

    psoDesc.InputLayout = { pInputElements, m_elementCount };

    if (!rootSignature) {
        printf("RootSignature�������ł��B\n");
        return;
    }

    psoDesc.pRootSignature = rootSignature->GetRootSignature();     //���[�g�V�O�l�`�����w��

    //�V�F�[�_�[��ݒ�
    ComPtr<ID3DBlob> pVsBlob, pPsBlob;
    HRESULT hr = 0;
    if (rootSignature->m_shaderKind == ShaderKinds::BoneShader) {
        hr = D3DReadFileToBlob(KeyString::SHADER_BONE_VERTEX, pVsBlob.GetAddressOf());
    }
    else if (rootSignature->m_shaderKind == ShaderKinds::PrimitiveShader) {
        hr = D3DReadFileToBlob(KeyString::SHADER_PRIMITIVE_VERTEX, pVsBlob.GetAddressOf());
    }
    else if (rootSignature->m_shaderKind == ShaderKinds::ShadowShader) {
        hr = D3DReadFileToBlob(KeyString::SHADER_SHADOW_VERTEX, pVsBlob.GetAddressOf());
    }
    if (FAILED(hr) || !pVsBlob)
    {
        printf("���_�V�F�[�_�[�̓ǂݍ��݂Ɏ��s�B�G���[�R�[�h:%lx\n", hr);
        return;
    }

    if (rootSignature->m_shaderKind == ShaderKinds::BoneShader) {
        hr = D3DReadFileToBlob(KeyString::SHADER_TEXTURE_PIXEL, pPsBlob.GetAddressOf());
    }
    else if (rootSignature->m_shaderKind == ShaderKinds::PrimitiveShader) {
        hr = D3DReadFileToBlob(KeyString::SHADER_PRIMITIVE_PIXEL, pPsBlob.GetAddressOf());
    }
    if (rootSignature->m_shaderKind != ShaderKinds::ShadowShader && (FAILED(hr) || !pPsBlob))
    {
        printf("�s�N�Z���V�F�[�_�[�̓ǂݍ��݂Ɏ��s�B�G���[�R�[�h:%lx\n", hr);
        return;
    }

    psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVsBlob.Get());
    if (rootSignature->m_shaderKind == ShaderKinds::ShadowShader) {
        psoDesc.PS = { nullptr, 0 };
    }
    else {
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPsBlob.Get());
    }

    //���X�^���C�U�[�X�e�[�g�i�f�t�H���g�j
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // �J�����O�͂Ȃ�

    //�u�����h�X�e�[�g�i�f�t�H���g�j
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    //�[�x�E�X�e���V���X�e�[�g�i�f�t�H���g�j
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    if (rootSignature->m_shaderKind == ShaderKinds::ShadowShader) {
        psoDesc.NumRenderTargets = 0;   //�e�̓����_�[�^�[�Q�b�g�s�v
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    }
    else {
        //�����_�[�^�[�Q�b�g�̐ݒ�i1�̃����_�[�^�[�Q�b�g�ARGBA8�t�H�[�}�b�g�j
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    }

    //�}���`�T���v�����O�̐ݒ�
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    //�p�C�v���C���X�e�[�g�̍쐬
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
    if (FAILED(hr)) {
        printf("�p�C�v���C���X�e�[�g�̍쐬�Ɏ��s���܂����B�G���[�R�[�h:%lx\n", hr);
    }

    delete[] pInputElements;
}

ID3D12PipelineState* PipelineState::GetPipelineState() const {
    return pipelineState.Get();
}

D3D12_INPUT_ELEMENT_DESC* PipelineState::GetInputElement(ShaderKinds shaderKind)
{
    if (shaderKind == ShaderKinds::BoneShader) {
        m_elementCount = 6;

        D3D12_INPUT_ELEMENT_DESC* pInputLayout = new D3D12_INPUT_ELEMENT_DESC[m_elementCount];
        pInputLayout[0] = D3D12_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[1] = D3D12_INPUT_ELEMENT_DESC{ "BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[2] = D3D12_INPUT_ELEMENT_DESC{ "BONEIDS", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[3] = D3D12_INPUT_ELEMENT_DESC{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[4] = D3D12_INPUT_ELEMENT_DESC{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[5] = D3D12_INPUT_ELEMENT_DESC{ "VERTEXID", 0, DXGI_FORMAT_R32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

        return pInputLayout;
    }
    else if (shaderKind == ShaderKinds::PrimitiveShader) {
        m_elementCount = 5;
        D3D12_INPUT_ELEMENT_DESC* pInputLayout = new D3D12_INPUT_ELEMENT_DESC[m_elementCount];
        pInputLayout[0] = D3D12_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[1] = D3D12_INPUT_ELEMENT_DESC{ "BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[2] = D3D12_INPUT_ELEMENT_DESC{ "BONEIDS", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[3] = D3D12_INPUT_ELEMENT_DESC{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[4] = D3D12_INPUT_ELEMENT_DESC{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

        return pInputLayout;
    }
    else if (shaderKind == ShaderKinds::ShadowShader) {
        m_elementCount = 3;
        D3D12_INPUT_ELEMENT_DESC* pInputLayout = new D3D12_INPUT_ELEMENT_DESC[m_elementCount];
        pInputLayout[0] = D3D12_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[1] = D3D12_INPUT_ELEMENT_DESC{ "BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[2] = D3D12_INPUT_ELEMENT_DESC{ "BONEIDS", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

        return pInputLayout;
    }
    return nullptr;
}
