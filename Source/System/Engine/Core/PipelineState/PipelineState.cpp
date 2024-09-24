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
    // 自動的にリソースが解放されます
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
    // シェーダーコンパイル
    ID3DBlob* vertexShader = nullptr;
    ID3DBlob* pixelShader = nullptr;

    // パイプラインステートの設定
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { /* Input Layout を指定 */ };
    psoDesc.pRootSignature = nullptr; // Root Signatureを指定
    //psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
    //psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1; // Render Target の数
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // Render Target のフォーマット
    psoDesc.SampleDesc.Count = 1; // MSAA サンプル数
    psoDesc.SampleDesc.Quality = 0; // MSAA の品質

    // 頂点シェーダー読み込み
    auto hr = D3DReadFileToBlob(vsFilePath.c_str(), &vertexShader);
    if (FAILED(hr))
    {
        printf("頂点シェーダーの読み込みに失敗");
        return;
    }
    // ピクセルシェーダー読み込み
    hr = D3DReadFileToBlob(psFilePath.c_str(), &pixelShader);
    if (FAILED(hr))
    {
        printf("ピクセルシェーダーの読み込みに失敗");
        return;
    }

    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);

    // パイプラインステートオブジェクトを作成
    hr = m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
    if (FAILED(hr)) {
        // エラーハンドリング
        printf("シェーダーの作成に失敗");
        return;
    }

    // シェーダーを解放
    if (vertexShader) vertexShader->Release();
    if (pixelShader) pixelShader->Release();

    m_bInited = true;
}
