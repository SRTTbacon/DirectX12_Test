#include "DescriptorHeap2.h"

DescriptorHeap::DescriptorHeap(ID3D12Device* device, UINT meshCount, UINT heapSize, ShadowSize shadowSize)
    : m_pDevice(device)
    , m_descriptorSize(0)
    , textureCount(0)
    , heapSize(heapSize)
    , m_shadowViewport(D3D12_VIEWPORT())
    , m_shadowScissorRect(D3D12_RECT())
    , m_cpuDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE())
    , m_gpuDescriptorHandle(D3D12_GPU_DESCRIPTOR_HANDLE())
    , m_shadowMapDSV(D3D12_CPU_DESCRIPTOR_HANDLE())
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = meshCount * heapSize;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descriptorHeap));
    m_descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    CreateShadowMap(shadowSize);
}

DescriptorHeap::~DescriptorHeap()
{
}

void DescriptorHeap::SetMainTexture(ID3D12Resource* mainTex, ID3D12Resource* normalMap, ID3D12Resource* pShadowMap)
{
    m_cpuDescriptorHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    m_gpuDescriptorHandle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    m_cpuDescriptorHandle.ptr += m_descriptorSize * static_cast<unsigned long long>(textureCount) * heapSize;
    m_gpuDescriptorHandle.ptr += m_descriptorSize * static_cast<unsigned long long>(textureCount) * heapSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = mainTex->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = mainTex->GetDesc().MipLevels;

    m_pDevice->CreateShaderResourceView(mainTex, &srvDesc, m_cpuDescriptorHandle);

    m_cpuDescriptorHandle.ptr += m_descriptorSize;
    m_gpuDescriptorHandle.ptr += m_descriptorSize;
    if (normalMap) {
        srvDesc.Format = normalMap->GetDesc().Format;
        srvDesc.Texture2D.MipLevels = normalMap->GetDesc().MipLevels;
        m_pDevice->CreateShaderResourceView(normalMap, &srvDesc, m_cpuDescriptorHandle);
    }

    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    m_cpuDescriptorHandle.ptr += m_descriptorSize;
    m_gpuDescriptorHandle.ptr += m_descriptorSize;

    if (pShadowMap) {
        m_pDevice->CreateShaderResourceView(pShadowMap, &srvDesc, m_cpuDescriptorHandle);
    }

    textureCount++;
}

void DescriptorHeap::CreateShadowMap(const ShadowSize shadowSize)
{
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = static_cast<UINT>(shadowSize);
    texDesc.Height = static_cast<UINT>(shadowSize);
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R32_TYPELESS;                //SRVおよびDSVと互換性を持つタイプレスフォーマット
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;    //深度ステンシルリソース用のフラグ

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    HRESULT hr = m_pDevice->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&m_shadowMap)
    );

    if (FAILED(hr)) {
        printf("シャドウマップの作成に失敗しました。エラーコード : %ld\n", hr);
        return;
    }

    //深度ステンシルビュー (DSV) 用ディスクリプタヒープの作成
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    m_pDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));

    m_shadowMapDSV = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

    //DSV の作成
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    m_pDevice->CreateDepthStencilView(m_shadowMap.Get(), &dsvDesc, m_shadowMapDSV);

    m_shadowViewport.TopLeftX = 0.0f;
    m_shadowViewport.TopLeftY = 0.0f;
    m_shadowViewport.Width = static_cast<float>(shadowSize);
    m_shadowViewport.Height = static_cast<float>(shadowSize);
    m_shadowViewport.MinDepth = 0.0f;
    m_shadowViewport.MaxDepth = 1.0f;

    //シザー矩形の設定
    m_shadowScissorRect.left = 0;
    m_shadowScissorRect.top = 0;
    m_shadowScissorRect.right = static_cast<LONG>(shadowSize);
    m_shadowScissorRect.bottom = static_cast<LONG>(shadowSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGpuDescriptorHandle(int index)
{
    D3D12_GPU_DESCRIPTOR_HANDLE  gpuDescriptorHandle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    gpuDescriptorHandle.ptr += m_descriptorSize * static_cast<unsigned long long>(index) * heapSize;
    return gpuDescriptorHandle;
}

ID3D12DescriptorHeap* DescriptorHeap::GetHeap() const
{
    return m_descriptorHeap.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE* DescriptorHeap::GetShadowMapDSV()
{
    return &m_shadowMapDSV;
}

D3D12_VIEWPORT* DescriptorHeap::GetShadowViewPort()
{
    return &m_shadowViewport;
}

D3D12_RECT* DescriptorHeap::GetShadowScissor()
{
    return &m_shadowScissorRect;
}

ID3D12Resource* DescriptorHeap::GetShadowMap()
{
    return m_shadowMap.Get();
}
