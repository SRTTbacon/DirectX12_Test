#include "DescriptorHeap2.h"

const unsigned long long DescriptorHeap::STATIC_RESOURCE_COUNT = 2;

DescriptorHeap::DescriptorHeap()
    : m_pDevice(nullptr)
    , m_descriptorSize(0)
{
}

DescriptorHeap::~DescriptorHeap()
{
}

void DescriptorHeap::Initialize(ID3D12Device* pDevice, UINT heapSize)
{
    m_pDevice = pDevice;

    //各マテリアルのテクスチャを収納するディスクリプタヒープを作成
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = heapSize;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descriptorHeap));

    //1つのヒープサイズ
    m_descriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void DescriptorHeap::SetResource(UINT offset, ID3D12Resource* pResource)
{
    SetResource(offset, pResource, pResource->GetDesc().Format);
}

void DescriptorHeap::SetResource(UINT offset, ID3D12Resource* pResource, DXGI_FORMAT format)
{
    if (!pResource) {
        return;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Format = format;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    SetResource(offset, pResource, srvDesc);
}

void DescriptorHeap::SetResource(UINT offset, ID3D12Resource* pResource, D3D12_SHADER_RESOURCE_VIEW_DESC& shaderResource)
{
    if (!pResource) {
        return;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    cpuDescriptorHandle.ptr += m_descriptorSize * static_cast<unsigned long long>(offset);
    m_pDevice->CreateShaderResourceView(pResource, &shaderResource, cpuDescriptorHandle);
}

void DescriptorHeap::SetMainTexture(UINT index, ID3D12Resource* mainTex, ID3D12Resource* normalMap, ID3D12Resource* pShapeBuffer)
{
    //各マテリアルにテクスチャを割り当てる

    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    cpuDescriptorHandle.ptr += m_descriptorSize * (static_cast<unsigned long long>(index * MATERIAL_DISCRIPTOR_HEAP_SIZE) + STATIC_RESOURCE_COUNT);

    //メインテクスチャ
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    if (mainTex) {
        srvDesc.Format = mainTex->GetDesc().Format;
        srvDesc.Texture2D.MipLevels = mainTex->GetDesc().MipLevels;
        m_pDevice->CreateShaderResourceView(mainTex, &srvDesc, cpuDescriptorHandle);
    }

    //1つ進めてノーマルマップ
    cpuDescriptorHandle.ptr += m_descriptorSize;
    if (normalMap) {
        srvDesc.Format = normalMap->GetDesc().Format;
        srvDesc.Texture2D.MipLevels = normalMap->GetDesc().MipLevels;
        m_pDevice->CreateShaderResourceView(normalMap, &srvDesc, cpuDescriptorHandle);
    }
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGpuDescriptorHandle(UINT index, UINT offset)
{
    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    //0番目はシャドウマップのためoffsetにSTATIC_RESOURCE_COUNTを足す
    gpuDescriptorHandle.ptr += m_descriptorSize * (static_cast<ULONG64>(index * MATERIAL_DISCRIPTOR_HEAP_SIZE + offset) + STATIC_RESOURCE_COUNT);
    return gpuDescriptorHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGpuDescriptorHandle(UINT offset)
{
    //offsetの位置にあるリソースを取得
    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    gpuDescriptorHandle.ptr += m_descriptorSize * (static_cast<ULONG64>(offset));
    return gpuDescriptorHandle;
}

ID3D12DescriptorHeap* DescriptorHeap::GetHeap() const
{
    return m_descriptorHeap.Get();
}
