#include "PipelineState.h"
#include <d3dcompiler.h>

PipelineState::PipelineState(ID3D12Device* pDevice, RootSignature* pRootSignature, D3D12_CULL_MODE cullMode)
    : m_elementCount(0)
{
    //パイプラインステートオブジェクトの作成
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

    //入力レイアウトの定義
    D3D12_INPUT_ELEMENT_DESC* pInputElements = GetInputElement(pRootSignature->GetShaderKind());

    psoDesc.InputLayout = { pInputElements, m_elementCount };

    if (!pRootSignature) {
        printf("RootSignatureが無効です。\n");
        return;
    }

    psoDesc.pRootSignature = pRootSignature->GetRootSignature();     //ルートシグネチャを指定

    //シェーダーを設定
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
        printf("頂点シェーダーの読み込みに失敗。エラーコード:%lx\n", hr);
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
        printf("ピクセルシェーダーの読み込みに失敗。エラーコード:%lx\n", hr);
        return;
    }

    psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVsBlob.Get());
    if (pRootSignature->GetShaderKind() == ShaderKinds::ShadowShader) {
        psoDesc.PS = { nullptr, 0 };
    }
    else {
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPsBlob.Get());
    }

    //ラスタライザーステート（デフォルト）
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = cullMode; //カリング

    //ブレンドステート（デフォルト）
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    //ブレンド状態の設定
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

    //深度・ステンシルステート（デフォルト）
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    //スカイボックスは深度バッファを書き込まないように
    if (pRootSignature->GetShaderKind() == ShaderKinds::SkyBoxShader) {
        //psoDesc.DepthStencilState.DepthEnable = true;
        //psoDesc.DepthStencilState.StencilEnable = false;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    }
    else if (pRootSignature->GetShaderKind() == ShaderKinds::UITextureShader) {
        //常に手前
        psoDesc.DepthStencilState.DepthEnable = false;
        psoDesc.DepthStencilState.StencilEnable = false;

        //UIは半透明対応
        SetAlphaBlend(psoDesc.BlendState);
    }
    else if (pRootSignature->GetShaderKind() == ShaderKinds::PostProcessShader) {
        psoDesc.DepthStencilState.DepthEnable = false;
        psoDesc.DepthStencilState.StencilEnable = false;
    }

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    if (pRootSignature->GetShaderKind() == ShaderKinds::ShadowShader) {
        psoDesc.NumRenderTargets = 0;   //影はレンダーターゲット不要
        psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    }
    else {
        //レンダーターゲットの設定（1つのレンダーターゲット、RGBA8フォーマット）
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    }

    //マルチサンプリングの設定
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    //パイプラインステートの作成
    hr = pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
    if (FAILED(hr)) {
        printf("パイプラインステートの作成に失敗しました。エラーコード:%lx\n", hr);
    }

    delete[] pInputElements;
}

ID3D12PipelineState* PipelineState::GetPipelineState() const {
    return pipelineState.Get();
}

void PipelineState::SetAlphaBlend(D3D12_BLEND_DESC& desc)
{
    //ブレンドを有効にする
    desc.RenderTarget[0].BlendEnable = TRUE;
    //ピクセルシェーダーが出力するRGB値に対してαを乗算する(SRCrgb ＊ SRCα)
    desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    //レンダーターゲットの現在のRGB値に対して1-αを乗算する(DESTrgb ＊ (1 ー SRCα))
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
