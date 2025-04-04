#pragma once

#include <DirectXMath.h>
#include <d3dx12.h>

#include "..\\..\\..\\ComPtr.h"

#include "..\\..\\Core\\Texture2D\\Texture2D.h"
#include "..\\..\\Core\\PipelineState\\PipelineState.h"
#include <dxgitype.h>

class UIElement
{
public:
    UIElement(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const SIZE* pScreenSize, DescriptorHeap* pDescriptorHeap, UINT descriptorIndex);
    ~UIElement();

    void Initialize(const ID3D12Resource* pVertexBuffer, const ID3D12Resource* pIndexBuffer, const D3D12_VERTEX_BUFFER_VIEW* pVertexBufferView, const D3D12_INDEX_BUFFER_VIEW* pIndexBufferView);

    virtual void Draw();

    virtual void Release();

public:
    inline UINT GetDescriptorIndex() const { return m_descriptorIndex; }

    inline bool GetIsReleased() const { return m_bReleased; }

public:
    //色 (Color::WHITEなどでも指定可)
    DXGI_RGBA m_color;
    //クライアント座標系 (x, y)
    DirectX::XMFLOAT2 m_position;
    //回転 (ラジアン角)
    DirectX::XMFLOAT3 m_rotation;
    //サイズ (テキストの場合は1文字の大きさ)
    DirectX::XMFLOAT2 m_size;

    //原点を中心にするかどうか
    //デフォルト : false
    bool m_bCenterPosition;

    //原点を真ん中にして回転するかどうか
    //デフォルト : true
    bool m_bCenterRotation;

protected:
    //頂点シェーダー用
    struct UIVertexBuffer
    {
        DirectX::XMFLOAT2 position;             //左上の座標 (ピクセル単位)
        DirectX::XMFLOAT2 size;                 //幅と高さ (ピクセル単位)
        DirectX::XMFLOAT2 screenSize;           //画面サイズ
        DirectX::XMFLOAT2 texCoordMin;          //uv座標の開始位置
        DirectX::XMFLOAT2 texCoordMax;          //uv座標の終了位置
        DirectX::XMMATRIX parentRotationMatrix; //回転用マトリックス
        DirectX::XMFLOAT2 parentRotationCenter; //回転中心座標
        DirectX::XMMATRIX childRotationMatrix;  //回転用マトリックス

    };
    //ピクセルシェーダー用
    struct UIPixelBuffer
    {
        DirectX::XMFLOAT4 color;
        UINT mode;
    };

    ID3D12Device* m_pDevice;
    ID3D12GraphicsCommandList* m_pCommandList;

    const ID3D12Resource* m_pVertexBuffer;
    const ID3D12Resource* m_pIndexBuffer;
    const D3D12_VERTEX_BUFFER_VIEW* m_pVertexBufferView;
    const D3D12_INDEX_BUFFER_VIEW* m_pIndexBufferView;

    DescriptorHeap* m_pDescriptorHeap;

    UIVertexBuffer m_vertexConstantBuffer;
    UIPixelBuffer m_pixelConstantBuffer;

    const SIZE* m_pScreenSize;
    const UINT m_descriptorIndex;

    bool m_bReleased;
};
