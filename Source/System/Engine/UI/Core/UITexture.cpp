#include "UITexture.h"
#include "..\\..\\Core\\XMFLOATHelper.h"

#include <filesystem>

using namespace DirectX;

UITexture::UITexture(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const SIZE* pScreenSize, DescriptorHeap* pDescriptorHeap, UINT descriptorIndex)
    : UIElement(pDevice, pCommandList, pScreenSize, pDescriptorHeap, descriptorIndex)
	, m_pTexture(nullptr)
{
	m_pTexture = Texture2D::GetColor(1.0f, 1.0f, 1.0f);

	m_pDescriptorHeap->SetResource(m_descriptorIndex, m_pTexture->Resource());

	m_vertexConstantBuffer.texCoordMin = XMFLOAT2(0.0f, 0.0f);
	m_vertexConstantBuffer.texCoordMax = XMFLOAT2(1.0f, 1.0f);

	m_pixelConstantBuffer.mode = 0;
}

UITexture::~UITexture()
{
	if (m_pTexture) {
		delete m_pTexture;
		m_pTexture = nullptr;
	}
}

void UITexture::SetTexture(std::string strFilePath)
{
	if (m_pTexture) {
		delete m_pTexture;
		m_pTexture = nullptr;
	}

	if (!std::filesystem::exists(strFilePath)) {
		throw DxSystemException::OM_TEXTUREMANAGE_NOTFOUND_ERROR;
	}

	m_pTexture = Texture2D::Get(strFilePath);

	m_pDescriptorHeap->SetResource(m_descriptorIndex, m_pTexture->Resource());
}

void UITexture::SetTexture(const char* data, size_t size)
{
	if (m_pTexture) {
		delete m_pTexture;
		m_pTexture = nullptr;
	}

	m_pTexture = Texture2D::Get(data, size);

	m_pDescriptorHeap->SetResource(m_descriptorIndex, m_pTexture->Resource());
}

void UITexture::SetTexture(DXGI_RGBA color)
{
	if (m_pTexture) {
		delete m_pTexture;
		m_pTexture = nullptr;
	}

	m_pTexture = Texture2D::GetColor(color.r, color.g, color.b, color.a);

	m_pDescriptorHeap->SetResource(m_descriptorIndex, m_pTexture->Resource());
}

void UITexture::SetTexture(Texture2D* pTexture)
{
	if (m_pTexture) {
		delete m_pTexture;
		m_pTexture = nullptr;
	}

	m_pDescriptorHeap->SetResource(m_descriptorIndex, pTexture->Resource());
}

void UITexture::Draw()
{
	//シェーダーに送信する変数
	m_vertexConstantBuffer.position = m_position;
	if (m_bCenterPosition) {
		m_vertexConstantBuffer.position -= m_size / 2.0f;
	}

	XMMATRIX rotX = XMMatrixRotationX(m_rotation.x);
	XMMATRIX rotY = XMMatrixRotationY(m_rotation.y);
	XMMATRIX rotZ = XMMatrixRotationZ(m_rotation.z);
	m_vertexConstantBuffer.parentRotationMatrix = rotX * rotY * rotZ;

	if (!m_bCenterRotation) {
		m_vertexConstantBuffer.parentRotationCenter -= m_size / 2.0f;
	}

	m_vertexConstantBuffer.size = m_size;                //幅と高さ
	m_pixelConstantBuffer.color = XMFLOAT4(m_color.r, m_color.g, m_color.b, m_color.a);

	m_pCommandList->SetGraphicsRootDescriptorTable(1, m_pDescriptorHeap->GetGpuDescriptorHandle(m_descriptorIndex));        //テクスチャ

	UIElement::Draw();
}

DirectX::XMFLOAT2 UITexture::GetTextureSize() const
{
	if (m_pTexture->Resource()) {
		return XMFLOAT2(static_cast<float>(m_pTexture->Resource()->GetDesc().Width), static_cast<float>(m_pTexture->Resource()->GetDesc().Height));
	}
	return XMFLOAT2(0.0f, 0.0f);
}
