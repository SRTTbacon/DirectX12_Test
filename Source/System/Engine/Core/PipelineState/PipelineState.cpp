#include "PipelineState.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

PipelineState::PipelineState(ID3D12Device* device, ID3D12RootSignature* rootSignature) {
    // パイプラインステートオブジェクトの作成
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

    // 入力レイアウトの定義
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
        printf("RootSignatureが無効です。\n");
        return; // エラーハンドリング
    }

    psoDesc.pRootSignature = rootSignature;

    // シェーダーバイナリを設定
    ComPtr<ID3DBlob> pVsBlob, pPsBlob;
    HRESULT hr = D3DReadFileToBlob(L"x64/Debug/SimpleVS.cso", pVsBlob.GetAddressOf());
    if (FAILED(hr) || !pVsBlob)
    {
        printf("頂点シェーダーの読み込みに失敗。エラーコード:%lx\n", hr);
        return;
    }
    hr = D3DReadFileToBlob(L"x64/Debug/SimplePS.cso", pPsBlob.GetAddressOf());
    if (FAILED(hr) || !pPsBlob)
    {
        printf("ピクセルシェーダーの読み込みに失敗。エラーコード:%lx\n", hr);
        return;
    }

    psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVsBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPsBlob.Get());

    // ラスタライザーステート（デフォルト）
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // カリングはなし

    // ブレンドステート（デフォルト）
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    // 深度・ステンシルステート（デフォルト）
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // レンダーターゲットの設定（1つのレンダーターゲット、RGBA8フォーマット）
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    // マルチサンプリングの設定
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    // パイプラインステートの作成
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
    if (FAILED(hr)) {
        printf("パイプラインステートの作成に失敗しました。エラーコード:%lx\n", hr);
    }
}

ID3D12PipelineState* PipelineState::GetPipelineState() const {
    return pipelineState.Get();
}
