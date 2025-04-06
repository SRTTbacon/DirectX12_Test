#include "GUILayout.h"

using namespace DirectX;

//----------GUIButton----------

GUIButton::GUIButton()
	: m_bClicked(false)
	, m_bMouseOver(false)
{
}

bool GUIButton::Update()
{
	//位置、大きさを設定
	m_normalTexture->m_position = m_position;
	m_mouseOverTexture->m_position = m_position;
	m_normalTexture->m_size = m_size;
	m_mouseOverTexture->m_size = m_size;

	m_bMouseOver = GetIsMouseOver();

	//クリック開始
	if (g_Engine->GetMouseStateSync(DIK_LBUTTON) && !m_bClicked && m_bMouseOver) {
		m_bClicked = true;
	}

	//クリックを離した際、ボタンの上にカーソルがあればtrueを返す
	if (m_bClicked) {
		if (!m_bMouseOver) {
			m_bClicked = false;
		}
		else if (g_Engine->GetMouseStateRelease(DIK_LBUTTON)) {
			m_bClicked = false;
			return true;
		}
	}

	return false;
}

void GUIButton::Draw() const
{
	if (!m_normalTexture || !m_mouseOverTexture) {
		return;
	}

	if (m_bMouseOver) {
		m_mouseOverTexture->Draw();
	}
	else {
		m_normalTexture->Draw();
	}

	if (m_text) {
		m_text->Draw();
	}
}

bool GUIButton::GetIsMouseOver() const
{
	POINT mousePos = g_Engine->GetMousePosition();

	int left = static_cast<int>(m_position.x - m_size.x / 2.0f);
	int top = static_cast<int>(m_position.y - m_size.y / 2.0f);
	int right = static_cast<int>(m_position.x + m_size.x / 2.0f);
	int bottom = static_cast<int>(m_position.y + m_size.y / 2.0f);

	return mousePos.x >= left && mousePos.y >= top && mousePos.x < right && mousePos.y < bottom;
}

//----------GUILayout----------

GUIButton GUILayout::CreateButton(std::string strNormalTexture, std::string strMouseOverTexture, XMFLOAT2 size)
{
	GUIButton button{};
	button.m_normalTexture = g_Engine->GetUIManager()->AddUITexture(strNormalTexture);
	button.m_mouseOverTexture = g_Engine->GetUIManager()->AddUITexture(strMouseOverTexture);
	button.m_text = nullptr;

	button.m_normalTexture->m_size = size;
	button.m_mouseOverTexture->m_size = size;

	button.m_size = size;

	return button;
}

GUIButton GUILayout::CreateButton(HCSReadFile& hcsFile, std::string strNormalTexture, std::string strMouseOverTexture, DirectX::XMFLOAT2 size)
{
	GUIButton button{};

	size_t normalTexSize;
	size_t mouseOverTexSize;
	char* pNormalTexData = hcsFile.GetFile(strNormalTexture, &normalTexSize);
	char* pMouseOverTexData = hcsFile.GetFile(strMouseOverTexture, &mouseOverTexSize);

	button.m_normalTexture = g_Engine->GetUIManager()->AddUITexture(pNormalTexData, normalTexSize);

	button.m_mouseOverTexture = g_Engine->GetUIManager()->AddUITexture(pMouseOverTexData, mouseOverTexSize);
	button.m_text = nullptr;

	button.m_normalTexture->m_size = size;
	button.m_mouseOverTexture->m_size = size;

	button.m_size = size;

	delete pNormalTexData;
	delete pMouseOverTexData;

	return button;
}

GUIAnimationText GUILayout::CreateAnimationText(std::string strFontPath, UINT fontSize, std::string text)
{
	GUIAnimationText animText{};
	UITextRef uiText= g_Engine->GetUIManager()->AddUIText(strFontPath, fontSize);
	uiText->m_text = text;
	uiText->m_color.a = 0.0f;

	animText.Initialize(uiText);

	return animText;
}

GUIAnimationTexture GUILayout::CreateAnimationTexture(std::string strTexturePath)
{
	GUIAnimationTexture animText{};
	UITextureRef uiText = g_Engine->GetUIManager()->AddUITexture(strTexturePath);
	uiText->m_color.a = 0.0f;

	animText.Initialize(uiText);

	return animText;
}

GUIAnimationTexture GUILayout::CreateAnimationTexture(HCSReadFile& hcsFile, std::string strTexturePath)
{
	size_t size;
	char* pTexBuffer = hcsFile.GetFile(strTexturePath, &size);

	GUIAnimationTexture animText{};
	UITextureRef uiText = g_Engine->GetUIManager()->AddUITexture(pTexBuffer, size);
	uiText->m_color.a = 0.0f;

	animText.Initialize(uiText);

	delete pTexBuffer;

	return animText;
}
