#include "PipelineState.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

PipelineState::PipelineState(ID3D12Device* device, RootSignature* rootSignature) 
    : m_elementCount(0)
{
    //パイプラインステートオブジェクトの作成
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

    //入力レイアウトの定義
    D3D12_INPUT_ELEMENT_DESC* pInputElements = GetInputElement(rootSignature->m_shaderKind);

    psoDesc.InputLayout = { pInputElements, m_elementCount };

    if (!rootSignature) {
        printf("RootSignatureが無効です。\n");
        return;
    }

    psoDesc.pRootSignature = rootSignature->GetRootSignature();     //ルートシグネチャを指定

    //シェーダーを設定
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
        printf("頂点シェーダーの読み込みに失敗。エラーコード:%lx\n", hr);
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
        printf("ピクセルシェーダーの読み込みに失敗。エラーコード:%lx\n", hr);
        return;
    }

    psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVsBlob.Get());
    if (rootSignature->m_shaderKind == ShaderKinds::ShadowShader) {
        psoDesc.PS = { nullptr, 0 };
    }
    else {
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPsBlob.Get());
    }

    //ラスタライザーステート（デフォルト）
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // カリングはなし

    //ブレンドステート（デフォルト）
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    //深度・ステンシルステート（デフォルト）
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    if (rootSignature->m_shaderKind == ShaderKinds::ShadowShader) {
        psoDesc.NumRenderTargets = 0;   //影はレンダーターゲット不要
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    }
    else {
        //レンダーターゲットの設定（1つのレンダーターゲット、RGBA8フォーマット）
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    }

    //マルチサンプリングの設定
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    //パイプラインステートの作成
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
    if (FAILED(hr)) {
        printf("パイプラインステートの作成に失敗しました。エラーコード:%lx\n", hr);
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
