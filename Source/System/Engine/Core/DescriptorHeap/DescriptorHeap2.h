#pragma once

#include <d3dx12.h>
#include <vector>
#include <stdexcept>
#include "..\\..\\..\\ComPtr.h"

enum ShadowSize
{
    ShadowSizeLow = 512,
    ShadowSizeMid = 1024,
    ShadowSizeHigh = 2048,
    ShadowSizeEpic = 4096
};

constexpr const UINT CHARACTER_DISCRIPTOR_HEAP_SIZE = 3;
constexpr const UINT MODEL_DISCRIPTOR_HEAP_SIZE = 3;

class DescriptorHeap
{
public:
    DescriptorHeap(ID3D12Device* device, UINT meshCount, UINT heapSize, ShadowSize shadowSize);
    ~DescriptorHeap();

    void SetMainTexture(ID3D12Resource* mainTex, ID3D12Resource* normalMap, ID3D12Resource* pShadowMap, ID3D12Resource* pShapeBuffer);

    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(UINT index, UINT offset = 0);
    ID3D12DescriptorHeap* GetHeap() const;
    D3D12_CPU_DESCRIPTOR_HANDLE* GetShadowMapDSV();
    D3D12_VIEWPORT* GetShadowViewPort();
    D3D12_RECT* GetShadowScissor();
    ID3D12Resource* GetShadowMap();

private:
    ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;
    UINT m_descriptorSize;

    ComPtr<ID3D12Resource> m_shadowMap;
    ID3D12Device* m_pDevice;

    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;  //DSV用のディスクリプタヒープ
    D3D12_CPU_DESCRIPTOR_HANDLE m_shadowMapDSV;

    D3D12_VIEWPORT m_shadowViewport;
    D3D12_RECT m_shadowScissorRect;

    int m_textureCount;
    int m_heapSize;

    void CreateShadowMap(const ShadowSize shadowSize);
};