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
    //�F (Color::WHITE�Ȃǂł��w���)
    DXGI_RGBA m_color;
    //�N���C�A���g���W�n (x, y)
    DirectX::XMFLOAT2 m_position;
    //��] (���W�A���p)
    DirectX::XMFLOAT3 m_rotation;
    //�T�C�Y (�e�L�X�g�̏ꍇ��1�����̑傫��)
    DirectX::XMFLOAT2 m_size;

    //���_�𒆐S�ɂ��邩�ǂ���
    //�f�t�H���g : false
    bool m_bCenterPosition;

    //���_��^�񒆂ɂ��ĉ�]���邩�ǂ���
    //�f�t�H���g : true
    bool m_bCenterRotation;

protected:
    //���_�V�F�[�_�[�p
    struct UIVertexBuffer
    {
        DirectX::XMFLOAT2 position;             //����̍��W (�s�N�Z���P��)
        DirectX::XMFLOAT2 size;                 //���ƍ��� (�s�N�Z���P��)
        DirectX::XMFLOAT2 screenSize;           //��ʃT�C�Y
        DirectX::XMFLOAT2 texCoordMin;          //uv���W�̊J�n�ʒu
        DirectX::XMFLOAT2 texCoordMax;          //uv���W�̏I���ʒu
        DirectX::XMMATRIX parentRotationMatrix; //��]�p�}�g���b�N�X
        DirectX::XMFLOAT2 parentRotationCenter; //��]���S���W
        DirectX::XMMATRIX childRotationMatrix;  //��]�p�}�g���b�N�X

    };
    //�s�N�Z���V�F�[�_�[�p
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
