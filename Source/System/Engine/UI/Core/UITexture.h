#pragma once

#include "UIElement.h"

class UITexture : public UIElement
{
public:
    UITexture(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const SIZE* pScreenSize, DescriptorHeap* pDescriptorHeap, UINT descriptorIndex);
    ~UITexture();

    //指定したファイルからテクスチャをセット
    void SetTexture(std::string strFilePath);
    void SetTexture(const char* data, size_t size);
    //単色のテクスチャをセット
    void SetTexture(DXGI_RGBA color);
    void SetTexture(Texture2D* pTexture);

    virtual void Draw();

public:
    inline ID3D12Resource* GetTextureResource() const { return m_pTexture->Resource(); }
    DirectX::XMFLOAT2 GetTextureSize() const;

private:
    Texture2D* m_pTexture;
};
