#pragma once

#include "..\\FileSystem\\HCSFileSystem.h"
#include "GUIAnimation.h"

class GUIButton : public GUIElement
{
public:
	GUIButton();

	UITextureRef m_normalTexture;
	UITextureRef m_mouseOverTexture;
	UITextRef m_text;

	//更新
	//クリックされたらtrue
	bool Update();

	void Draw() const;

private:
	bool m_bClicked;
	bool m_bMouseOver;

	bool GetIsMouseOver() const;
};

class GUILayout
{
public:
	static GUIButton CreateButton(std::string strNormalTexture, std::string strMouseOverTexture, DirectX::XMFLOAT2 size);
	static GUIButton CreateButton(HCSReadFile& hcsFile, std::string strNormalTexture, std::string strMouseOverTexture, DirectX::XMFLOAT2 size);
	static GUIAnimationText CreateAnimationText(std::string strFontPath, UINT fontSize, std::string text);
	static GUIAnimationTexture CreateAnimationTexture(std::string strTexturePath);
	static GUIAnimationTexture CreateAnimationTexture(HCSReadFile& hcsFile, std::string strTexturePath);
};