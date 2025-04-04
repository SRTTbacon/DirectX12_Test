#pragma once

#include "UIElement.h"
#include "FontManager\\FontManager.h"

class UIText : public UIElement
{
public:
	UIText(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const SIZE* pScreenSize, DescriptorHeap* pDescriptorHeap, UINT descriptorIndex, FontRef font);

	void SetTextDistance(DirectX::XMFLOAT2 distance);

	virtual void Draw();

public:
	std::string m_text;

public:
	inline DirectX::XMFLOAT2 GetTextDistance() const { return m_adjustPos; }

private:
	FontRef m_font;

	DirectX::XMFLOAT2 m_adjustPos;	//ï∂éöìØémÇÃïù
};
