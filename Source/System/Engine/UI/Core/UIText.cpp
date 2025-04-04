#include "UIText.h"
#include "..\\..\\Core\\XMFLOATHelper.h"

using namespace DirectX;

UIText::UIText(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const SIZE* pScreenSize, DescriptorHeap* pDescriptorHeap, UINT descriptorIndex, FontRef font)
	: UIElement(pDevice, pCommandList, pScreenSize, pDescriptorHeap, descriptorIndex)
	, m_font(font)
	, m_adjustPos(XMFLOAT2(0.0f, 0.0f))
{
	m_pDescriptorHeap->SetResource(descriptorIndex, font->GetResource());

	m_pixelConstantBuffer.mode = 1;
}

void UIText::SetTextDistance(DirectX::XMFLOAT2 distance)
{
	m_adjustPos = distance;
}

void UIText::Draw()
{
	bool bAdded = false;
	std::wstring wText = GetWideString(m_text);

	XMFLOAT2 rightBottomRelativePos = XMFLOAT2(0.0f, 0.0f);
	float tempRightRelativePos = 0.0f;

	//使用する文字が追加されていなければ追加
	//事前にテキストの中央位置を計算
	for (wchar_t& c : wText) {
		if (c == u'\0') {
			continue;
		}

		if (c == u'\n') {
			if (tempRightRelativePos > rightBottomRelativePos.x) {
				rightBottomRelativePos.x = tempRightRelativePos;
			}
			rightBottomRelativePos.y += static_cast<float>(m_font->GetFontSize() / 2) * m_size.y + m_adjustPos.y;
			tempRightRelativePos = 0.0f;

			continue;
		}

		if (c == u' ') {
			tempRightRelativePos += (m_font->GetFontSize() * m_size.x / 2.0f) + m_adjustPos.x;
			continue;
		}
		else if (c == u'　') {
			tempRightRelativePos += (m_font->GetFontSize() * m_size.x) + m_adjustPos.x;
			continue;
		}

		bool temp = m_font->AddCharacter(c);
		if (temp) {
			bAdded = true;
		}

		Font::GlyphInfo glyph = m_font->GetCharGlyph(c);

		tempRightRelativePos += (glyph.width + glyph.left) * m_size.x + m_adjustPos.x;
	}
	if (tempRightRelativePos > rightBottomRelativePos.x) {
		rightBottomRelativePos.x = tempRightRelativePos;
	}

	if (bAdded) {
		//文字が追加されたらテクスチャを更新
		m_font->UpdateAtlas();
	}

	//文字のテクスチャ
	m_pCommandList->SetGraphicsRootDescriptorTable(2, m_pDescriptorHeap->GetGpuDescriptorHandle(m_descriptorIndex));

	XMFLOAT2 relativePos = XMFLOAT2(0.0f, 0.0f);

	for (wchar_t& c : wText) {
		if (c == u'\0') {
			continue;
		}

		if (c == u' ') {
			relativePos.x += (m_font->GetFontSize() * m_size.x / 2.0f) + m_adjustPos.x;
			continue;
		}
		else if (c == u'　') {
			relativePos.x += (m_font->GetFontSize() * m_size.x) + m_adjustPos.x;
			continue;
		}

		Font::GlyphInfo glyph = m_font->GetCharGlyph(c);
		//printf("x = % d, y = % d\n", &c, glyph.x, glyph.y);
		float top = -(static_cast<float>(glyph.top) - m_font->GetFontSize() / 2.0f) * m_size.y;
		m_vertexConstantBuffer.position = m_position + relativePos + XMFLOAT2(static_cast<float>(glyph.left), top);
		if (m_bCenterPosition) {
			m_vertexConstantBuffer.position += -rightBottomRelativePos / 2.0f;
		}
		m_vertexConstantBuffer.size = XMFLOAT2(static_cast<float>(glyph.width), static_cast<float>(glyph.height)) * m_size;                //幅と高さ
		XMMATRIX rotX = XMMatrixRotationX(m_rotation.x);
		XMMATRIX rotY = XMMatrixRotationY(m_rotation.y);
		XMMATRIX rotZ = XMMatrixRotationZ(m_rotation.z);
		m_vertexConstantBuffer.parentRotationMatrix = rotX * rotY * rotZ;
		m_vertexConstantBuffer.parentRotationCenter = -relativePos * 2.0f + XMFLOAT2(-static_cast<float>(glyph.width + glyph.left * 1.75f), static_cast<float>(glyph.top)) * m_size;
		if (m_bCenterRotation) {
			m_vertexConstantBuffer.parentRotationCenter += rightBottomRelativePos;
		}
		m_vertexConstantBuffer.texCoordMin = XMFLOAT2(glyph.uvMinX, glyph.uvMinY);
		m_vertexConstantBuffer.texCoordMax = XMFLOAT2(glyph.uvMaxX, glyph.uvMaxY);

		m_pixelConstantBuffer.color = XMFLOAT4(m_color.r, m_color.g, m_color.b, m_color.a);

		UIElement::Draw();

		if (c != u'\n') {
			relativePos.x += glyph.width * m_size.x + m_adjustPos.x + static_cast<float>(glyph.left);
		}
		else {
			relativePos.x = 0;
			relativePos.y += m_font->GetFontSize() * m_size.y + m_adjustPos.y;
		}
	}
}
