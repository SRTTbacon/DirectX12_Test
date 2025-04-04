#pragma once

#include "Core\\FontManager\\FontManager.h"

#include "Core\\UITexture.h"
#include "Core\\UIText.h"

using UITextureRef = std::shared_ptr<UITexture>;
using UITextRef = std::shared_ptr<UIText>;
using UIElementRef = std::shared_ptr<UIElement>;

class UIManager
{
    friend class Engine;
public:
    UIManager();
	void Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, SIZE* pScreenSize);

    UITextureRef AddUITexture();
    UITextureRef AddUITexture(std::string strFilePath);
    UITextureRef AddUITexture(const char* data, size_t size);
    UITextureRef AddUITexture(DXGI_RGBA color);

    UITextRef AddUIText(std::string strFontPath, UINT fontSize);

private:
    struct UIVertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT2 uv;
    };

    static const UINT DESCRIPTOR_COUNT;

    std::vector<UIElementRef> m_elements;
    std::vector<bool> m_bCanAccessDescriptorIndexes;

    ID3D12Device* m_pDevice;
    ID3D12GraphicsCommandList* m_pCommandList;

    SIZE* m_pScreenSize;

    ComPtr<ID3D12Resource> m_vertexBuffer;
    ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;      //頂点バッファのデータ内容とサイズを保持
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;        //インデックスバッファのデータ内容とサイズを保持

    RootSignature m_uiRootSignature;
    PipelineState m_uiPipelineState;

    DescriptorHeap m_descriptorHeap;
    FontManager m_fontManager;

    UINT m_nextDescriptorIndex;

    void CreateVertexBuffer();
    void CreateIndexBuffer();

    void BeginRender();

    void Update();

    UINT GetCanAccessDescriptorIndex();
};
