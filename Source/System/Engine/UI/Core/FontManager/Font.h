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

	//������ǉ�
	bool AddCharacter(wchar_t& character);

	void UpdateAtlas();

	void Release() const;

	//UV�Ǘ��p�̃f�[�^�\����
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

	//�A�g���X�̊Ǘ�
	UINT m_atlasWidth;	//�����A�g���X��
	UINT m_atlasHeight;	//�����A�g���X����
	UINT m_currentX;		//�A�g���X���̌��݂�X�ʒu
	UINT m_currentY;		//�A�g���X���̌��݂�Y�ʒu
	UINT m_rowHeight;		//���ݍs�̍ő卂��

	//�O���t�����Ǘ�����z��
	std::unordered_map<wchar_t, GlyphInfo> m_glyphs;

	//���łɒǉ�����������ǐՂ��邽�߂̃Z�b�g
	std::unordered_set<wchar_t> m_addedCharacters;

	void* m_mappedData = nullptr;

	void CreateAtlasBuffer();
	void CreateUploadBuffer();
};
