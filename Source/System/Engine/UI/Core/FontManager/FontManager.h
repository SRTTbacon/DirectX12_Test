#pragma once

#include <unordered_map>

#include "Font.h"

#undef CreateFont

using FontRef = std::shared_ptr<Font>;

class FontManager
{
public:
	FontManager();

	void Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList);

	FontRef CreateFont(std::string strFilePath, UINT fontSize);

private:
	ID3D12Device* m_pDevice;
	ID3D12GraphicsCommandList* m_pCommandList;

	FT_Library m_library;

	std::unordered_map<std::string, FontRef> m_registerdFonts;
};
