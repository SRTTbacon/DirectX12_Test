#pragma once

#include <d3dx12.h>
#include <vector>
#include <stdexcept>
#include "..\\..\\..\\ComPtr.h"

constexpr const UINT MAX_DESCRIPTORHEAP_SIZE = 10000;       //�V�[���S�̂Ŏg�p����e�N�X�`�����̍ő� (�]�T�������Ċm�ۂ���B10000��312kb���x)
constexpr const UINT MATERIAL_DISCRIPTOR_HEAP_SIZE = 2;     //1�̃}�e���A���Ŏg�p����e�N�X�`����
constexpr const UINT MAX_MATERIAL_COUNT = MAX_DESCRIPTORHEAP_SIZE / (MATERIAL_DISCRIPTOR_HEAP_SIZE + 1) - 1;

constexpr const DXGI_FORMAT SHADOWMAP_FORMAT = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
constexpr const DXGI_FORMAT SHAPE_FORMAT = DXGI_FORMAT_R32G32B32A32_FLOAT;

class DescriptorHeap
{
public:
    DescriptorHeap();
    ~DescriptorHeap();

    void Initialize(ID3D12Device* pDevice, UINT heapSize);
    void SetResource(UINT offset, ID3D12Resource* pResource, DXGI_FORMAT format);
    void SetMainTexture(UINT index, ID3D12Resource* mainTex, ID3D12Resource* normalMap, ID3D12Resource* pShapeBuffer);

    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(UINT index, UINT offset);
    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(UINT offset);
    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle();
    ID3D12DescriptorHeap* GetHeap() const;

private:
    static const unsigned long long STATIC_RESOURCE_COUNT; //�ϓ����������\�[�X���������݂��邩

    ID3D12Device* m_pDevice;

    ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;

    UINT m_descriptorSize;
};