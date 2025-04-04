#include "UIElement.h"

using namespace DirectX;

UIElement::UIElement(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const SIZE* pScreenSize, DescriptorHeap* pDescriptorHeap, UINT descriptorIndex)
	: m_pDevice(pDevice)
	, m_pCommandList(pCommandList)
	, m_position(XMFLOAT2(0.0f, 0.0f))
	, m_rotation(XMFLOAT3(0.0f, 0.0f, 0.0f))
	, m_size(0.0f, 0.0f)
	, m_pScreenSize(pScreenSize)
	, m_pDescriptorHeap(pDescriptorHeap)
	, m_descriptorIndex(descriptorIndex)
	, m_pVertexBuffer(nullptr)
	, m_pIndexBuffer(nullptr)
	, m_pVertexBufferView(nullptr)
	, m_pIndexBufferView(nullptr)
	, m_color(DXGI_RGBA(1.0f, 1.0f, 1.0f, 1.0f))
	, m_bReleased(false)
	, m_bCenterPosition(false)
	, m_bCenterRotation(true)
	, m_vertexConstantBuffer(UIVertexBuffer())
	, m_pixelConstantBuffer(UIPixelBuffer())
{
	m_vertexConstantBuffer.parentRotationMatrix = XMMatrixIdentity();
	m_vertexConstantBuffer.childRotationMatrix = XMMatrixIdentity();
	m_vertexConstantBuffer.parentRotationCenter = XMFLOAT2(0.0f, 0.0f);
}

UIElement::~UIElement()
{
}

void UIElement::Initialize(const ID3D12Resource* pVertexBuffer, const ID3D12Resource* pIndexBuffer, const D3D12_VERTEX_BUFFER_VIEW* pVertexBufferView, const D3D12_INDEX_BUFFER_VIEW* pIndexBufferView)
{
	m_pVertexBuffer = pVertexBuffer;
	m_pIndexBuffer = pIndexBuffer;
	m_pVertexBufferView = pVertexBufferView;
	m_pIndexBufferView = pIndexBufferView;
}

void UIElement::Draw()
{
	m_vertexConstantBuffer.screenSize.x = static_cast<float>(m_pScreenSize->cx);	//ウィンドウサイズ
	m_vertexConstantBuffer.screenSize.y = static_cast<float>(m_pScreenSize->cy);	//ウィンドウサイズ

	//頂点シェーダー用の変数をGPUに送信
	m_pCommandList->SetGraphicsRoot32BitConstants(0, sizeof(m_vertexConstantBuffer) / 4, &m_vertexConstantBuffer, 0);

	//ピクセルシェーダー用の変数をGPUに送信
	m_pCommandList->SetGraphicsRoot32BitConstants(3, sizeof(UIPixelBuffer) / 4, &m_pixelConstantBuffer, 0);

	m_pCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void UIElement::Release()
{
	m_bReleased = true;
}
