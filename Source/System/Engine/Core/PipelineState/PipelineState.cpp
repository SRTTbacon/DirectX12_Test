#include "PipelineState.h"
#include <d3dcompiler.h>

PipelineState::PipelineState(ID3D12Device* pDevice, RootSignature* pRootSignature, D3D12_CULL_MODE cullMode)
    : m_elementCount(0)
{
    //�p�C�v���C���X�e�[�g�I�u�W�F�N�g�̍쐬
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

    //���̓��C�A�E�g�̒�`
    D3D12_INPUT_ELEMENT_DESC* pInputElements = GetInputElement(pRootSignature->GetShaderKind());

    psoDesc.InputLayout = { pInputElements, m_elementCount };

    if (!pRootSignature) {
        printf("RootSignature�������ł��B\n");
        return;
    }

    psoDesc.pRootSignature = pRootSignature->GetRootSignature();     //���[�g�V�O�l�`�����w��

    //�V�F�[�_�[��ݒ�
    ComPtr<ID3DBlob> pVsBlob, pPsBlob;
    HRESULT hr = 0;
    if (pRootSignature->GetShaderKind() == ShaderKinds::BoneShader) {
        hr = D3DReadFileToBlob(KeyString::SHADER_BONE_VERTEX, &pVsBlob);
    }
    else if (pRootSignature->GetShaderKind() == ShaderKinds::PrimitiveShader || pRootSignature->GetShaderKind() == ShaderKinds::TerrainShader) {
        hr = D3DReadFileToBlob(KeyString::SHADER_PRIMITIVE_VERTEX, &pVsBlob);
    }
    else if (pRootSignature->GetShaderKind() == ShaderKinds::GrassShader) {
        hr = D3DReadFileToBlob(KeyString::SHADER_GRASS_VERTEX, &pVsBlob);
    }
    else if (pRootSignature->GetShaderKind() == ShaderKinds::ShadowShader) {
        hr = D3DReadFileToBlob(KeyString::SHADER_SHADOW_VERTEX, &pVsBlob);
    }
    else if (pRootSignature->GetShaderKind() == ShaderKinds::SkyBoxShader) {
        hr = D3DReadFileToBlob(KeyString::SHADER_SKYBOX_VERTEX, &pVsBlob);
    }
    else if (pRootSignature->GetShaderKind() == ShaderKinds::UITextureShader) {
        hr = D3DReadFileToBlob(KeyString::SHADER_UITEXTURE_VERTEX, &pVsBlob);
    }
    else if (pRootSignature->GetShaderKind() == ShaderKinds::PostProcessShader) {
        hr = D3DReadFileToBlob(KeyString::SHADER_POSTPROCESS_VERTEX, &pVsBlob);
    }
    if (FAILED(hr) || !pVsBlob)
    {
        printf("���_�V�F�[�_�[�̓ǂݍ��݂Ɏ��s�B�G���[�R�[�h:%lx\n", hr);
        return;
    }

    if (pRootSignature->GetShaderKind() == ShaderKinds::BoneShader) {
        hr = D3DReadFileToBlob(KeyString::SHADER_TEXTURE_PIXEL, &pPsBlob);
    }
    else if (pRootSignature->GetShaderKind() == ShaderKinds::PrimitiveShader) {
        hr = D3DReadFileToBlob(KeyString::SHADER_PRIMITIVE_PIXEL, &pPsBlob);
    }
    else if (pRootSignature->GetShaderKind() == ShaderKinds::TerrainShader) {
        hr = D3DReadFileToBlob(KeyString::SHADER_TERRAIN_PIXEL, &pPsBlob);
    }
    else if (pRootSignature->GetShaderKind() == ShaderKinds::GrassShader) {
        hr = D3DReadFileToBlob(KeyString::SHADER_GRASS_PIXEL, &pPsBlob);
    }
    else if (pRootSignature->GetShaderKind() == ShaderKinds::SkyBoxShader) {
        hr = D3DReadFileToBlob(KeyString::SHADER_SKYBOX_PIXEL, &pPsBlob);
    }
    else if (pRootSignature->GetShaderKind() == ShaderKinds::UITextureShader) {
        hr = D3DReadFileToBlob(KeyString::SHADER_UITEXTURE_PIXEL, &pPsBlob);
    }
    else if (pRootSignature->GetShaderKind() == ShaderKinds::PostProcessShader) {
        hr = D3DReadFileToBlob(KeyString::SHADER_POSTPROCESS_PIXEL, &pPsBlob);
    }
    if (pRootSignature->GetShaderKind() != ShaderKinds::ShadowShader && (FAILED(hr) || !pPsBlob))
    {
        printf("�s�N�Z���V�F�[�_�[�̓ǂݍ��݂Ɏ��s�B�G���[�R�[�h:%lx\n", hr);
        return;
    }

    psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVsBlob.Get());
    if (pRootSignature->GetShaderKind() == ShaderKinds::ShadowShader) {
        psoDesc.PS = { nullptr, 0 };
    }
    else {
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPsBlob.Get());
    }

    //���X�^���C�U�[�X�e�[�g�i�f�t�H���g�j
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = cullMode; //�J�����O

    //�u�����h�X�e�[�g�i�f�t�H���g�j
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    //�u�����h��Ԃ̐ݒ�
    //psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
    //psoDesc.BlendState.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
    {
        FALSE,FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
        psoDesc.BlendState.RenderTarget[i] = defaultRenderTargetBlendDesc;
    }

    //�[�x�E�X�e���V���X�e�[�g�i�f�t�H���g�j
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    //�X�J�C�{�b�N�X�͐[�x�o�b�t�@���������܂Ȃ��悤��
    if (pRootSignature->GetShaderKind() == ShaderKinds::SkyBoxShader) {
        //psoDesc.DepthStencilState.DepthEnable = true;
        //psoDesc.DepthStencilState.StencilEnable = false;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    }
    else if (pRootSignature->GetShaderKind() == ShaderKinds::UITextureShader) {
        //��Ɏ�O
        psoDesc.DepthStencilState.DepthEnable = false;
        psoDesc.DepthStencilState.StencilEnable = false;

        //UI�͔������Ή�
        SetAlphaBlend(psoDesc.BlendState);
    }
    else if (pRootSignature->GetShaderKind() == ShaderKinds::PostProcessShader) {
        psoDesc.DepthStencilState.DepthEnable = false;
        psoDesc.DepthStencilState.StencilEnable = false;
    }

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    if (pRootSignature->GetShaderKind() == ShaderKinds::ShadowShader) {
        psoDesc.NumRenderTargets = 0;   //�e�̓����_�[�^�[�Q�b�g�s�v
        psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    }
    else {
        //�����_�[�^�[�Q�b�g�̐ݒ�i1�̃����_�[�^�[�Q�b�g�ARGBA8�t�H�[�}�b�g�j
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    }

    //�}���`�T���v�����O�̐ݒ�
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    //�p�C�v���C���X�e�[�g�̍쐬
    hr = pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
    if (FAILED(hr)) {
        printf("�p�C�v���C���X�e�[�g�̍쐬�Ɏ��s���܂����B�G���[�R�[�h:%lx\n", hr);
    }

    delete[] pInputElements;
}

ID3D12PipelineState* PipelineState::GetPipelineState() const {
    return pipelineState.Get();
}

void PipelineState::SetAlphaBlend(D3D12_BLEND_DESC& desc)
{
    //�u�����h��L���ɂ���
    desc.RenderTarget[0].BlendEnable = TRUE;
    //�s�N�Z���V�F�[�_�[���o�͂���RGB�l�ɑ΂��ă�����Z����(SRCrgb �� SRC��)
    desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    //�����_�[�^�[�Q�b�g�̌��݂�RGB�l�ɑ΂���1-������Z����(DESTrgb �� (1 �[ SRC��))
    desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
}

D3D12_INPUT_ELEMENT_DESC* PipelineState::GetInputElement(ShaderKinds shaderKind)
{
    if (shaderKind == ShaderKinds::BoneShader || shaderKind == ShaderKinds::PrimitiveShader || shaderKind == ShaderKinds::TerrainShader || shaderKind == ShaderKinds::GrassShader) {
        m_elementCount = 8;

        D3D12_INPUT_ELEMENT_DESC* pInputLayout = new D3D12_INPUT_ELEMENT_DESC[m_elementCount];
        pInputLayout[0] = D3D12_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[1] = D3D12_INPUT_ELEMENT_DESC{ "BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[2] = D3D12_INPUT_ELEMENT_DESC{ "BONEIDS", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[3] = D3D12_INPUT_ELEMENT_DESC{ "VERTEXID", 0, DXGI_FORMAT_R32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[4] = D3D12_INPUT_ELEMENT_DESC{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[5] = D3D12_INPUT_ELEMENT_DESC{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[6] = D3D12_INPUT_ELEMENT_DESC{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[7] = D3D12_INPUT_ELEMENT_DESC{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

        return pInputLayout;
    }
    else if (shaderKind == ShaderKinds::ShadowShader) {
        m_elementCount = 4;
        D3D12_INPUT_ELEMENT_DESC* pInputLayout = new D3D12_INPUT_ELEMENT_DESC[m_elementCount];
        pInputLayout[0] = D3D12_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[1] = D3D12_INPUT_ELEMENT_DESC{ "BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[2] = D3D12_INPUT_ELEMENT_DESC{ "BONEIDS", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[3] = D3D12_INPUT_ELEMENT_DESC{ "VERTEXID", 0, DXGI_FORMAT_R32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

        return pInputLayout;
    }
    else if (shaderKind == ShaderKinds::SkyBoxShader) {
        m_elementCount = 1;
        D3D12_INPUT_ELEMENT_DESC* pInputLayout = new D3D12_INPUT_ELEMENT_DESC[m_elementCount];
        pInputLayout[0] = D3D12_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

        return pInputLayout;
    }
    else if (shaderKind == ShaderKinds::UITextureShader || shaderKind == ShaderKinds::PostProcessShader) {
        m_elementCount = 2;
        D3D12_INPUT_ELEMENT_DESC* pInputLayout = new D3D12_INPUT_ELEMENT_DESC[m_elementCount];
        pInputLayout[0] = D3D12_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        pInputLayout[1] = D3D12_INPUT_ELEMENT_DESC{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

        return pInputLayout;
    }
    return nullptr;
}
