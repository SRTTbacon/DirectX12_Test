#pragma once
#include <unordered_set>
#include <unordered_map>
#include <ft2build.h>
#include <d3dx12.h>

#include "..\\..\\..\\Core\\ResourceCopy\\ResourceCopy.h"
#include "..\\..\\..\\..\\ComPtr.h"

#include FT_FREETYPE_H

constexpr const UINT ATLAS_DEFAULT_SIZE = 2048;

class Font
{
	friend class FontManager;
public:
	Font(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const std::string strFilePath, UINT fontSize);
	~Font();

	//文字を追加
	bool AddCharacter(wchar_t& character);

	void UpdateAtlas();

	void Release() const;

	//UV管理用のデータ構造体
	struct GlyphInfo
	{
		UINT x;
		UINT y;
		UINT width;
		UINT height;
		UINT left;
		UINT top;
		float uvMinX;
		float uvMinY;
		float uvMaxX;
		float uvMaxY;
	};

public:
	inline UINT GetFontSize() const { return m_fontSize; }

	inline ID3D12Resource* GetResource() const { return m_atlasResource.Get(); }

	inline GlyphInfo GetCharGlyph(wchar_t c)
	{
		if (m_glyphs.find(c) == m_glyphs.end()) {
			return GlyphInfo();
		}
		return m_glyphs[c];
	}

private:
	ID3D12Device* m_pDevice;
	ID3D12GraphicsCommandList* m_pCommandList;

	FT_Face m_fontFace;
	ComPtr<ID3D12Resource> m_atlasResource;
	ComPtr<ID3D12Resource> m_uploadBuffer;

	const std::string m_fontFile;

	const UINT m_fontSize;

	//アトラスの管理
	UINT m_atlasWidth;	//初期アトラス幅
	UINT m_atlasHeight;	//初期アトラス高さ
	UINT m_currentX;		//アトラス内の現在のX位置
	UINT m_currentY;		//アトラス内の現在のY位置
	UINT m_rowHeight;		//現在行の最大高さ

	//グリフ情報を管理する配列
	std::unordered_map<wchar_t, GlyphInfo> m_glyphs;

	//すでに追加した文字を追跡するためのセット
	std::unordered_set<wchar_t> m_addedCharacters;

	void* m_mappedData = nullptr;

	void CreateAtlasBuffer();
	void CreateUploadBuffer();
};
