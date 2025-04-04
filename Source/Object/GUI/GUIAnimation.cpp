#include "GUIAnimation.h"

using namespace DirectX;

//----------GUIElement----------

GUIElement::GUIElement()
	: m_position(XMFLOAT2(0.0f, 0.0f))
	, m_size(XMFLOAT2(0.0f, 0.0f))
{
}

GUIAnimation::GUIAnimation()
	: m_fadeInTime(1.0f)
	, m_fadeOutTime(1.0f)
	, m_intervalTime(2.0f)
	, m_continueTime(0.0f)
	, m_animTime(-1.0f)
	, m_nextScale(1.0f)
	, m_nextAnimationTemp(nullptr)
{
}

void GUIAnimation::Initialize(UIElementRef uiElement)
{
	m_uiElement = uiElement;
}

void GUIAnimation::RemoveNextAnimation()
{
	m_nextAnimationTemp = nullptr;
	m_nextScale = 1.0f;
}

void GUIAnimation::Update()
{
	if (!m_uiElement) {
		return;
	}

	//テキストの更新
	m_uiElement->m_position = m_position;

	//アニメーション中
	if (m_animTime >= 0.0f) {
		m_animTime += g_Engine->GetFrameTime();

		if (m_animTime < m_continueTime) {
			m_uiElement->m_color.a = 0.0f;
			return;
		}

		float startFadeOutTime = m_fadeInTime + m_intervalTime;

		float value;
		//フェードアウト
		if (m_animTime >= startFadeOutTime) {
			value = m_animTime - startFadeOutTime;
			value /= m_fadeOutTime;
			value = (1.0f - value);

			//アニメーション終了
			if (value <= 0.0f) {
				//次のアニメーションがあれば次へ
				if (m_nextAnimationTemp) {
					if (m_nextAnimationTemp->m_uiElement) {
						m_uiElement = m_nextAnimationTemp->m_uiElement;
					}
					m_fadeInTime = m_nextAnimationTemp->m_fadeInTime;
					m_fadeOutTime = m_nextAnimationTemp->m_fadeOutTime;
					m_intervalTime = m_nextAnimationTemp->m_intervalTime;
					m_continueTime = m_nextAnimationTemp->m_continueTime;
					m_animTime = 0.0f;
					SetNextAnimation();
					RemoveNextAnimation();
				}
				else {
					m_animTime = -1.0f;
				}
			}
		}
		//表示中
		else if (m_animTime >= m_fadeInTime) {
			value = 1.0f;
		}
		//フェードイン
		else {
			value = m_animTime / m_fadeInTime;
		}

		//0.0f～1.0fに修正
		value = max(0.0f, value);
		value = std::min(1.0f, value);

		m_uiElement->m_color.a = value;
	}
}

void GUIAnimation::Draw()
{
	if (!m_uiElement || m_uiElement->m_color.a <= 0.0f) {
		return;
	}

	m_uiElement->Draw();
}

void GUIAnimation::StartAnimation()
{
	m_animTime = 0.0f;
}

//----------GUIAnimationText----------

GUIAnimationText::GUIAnimationText()
	: m_nextAnimation(nullptr)
{
	m_size = XMFLOAT2(1.0f, 1.0f);
}

void GUIAnimationText::SetNextAnimation(std::string text, float scale, float continueTime)
{
	std::shared_ptr<GUIAnimationText> nextAnim = std::make_shared<GUIAnimationText>();
	nextAnim->m_continueTime = continueTime;
	m_nextText = text;
	m_nextScale = scale;

	m_nextAnimation = nextAnim;
	m_nextAnimationTemp = m_nextAnimation.get();
}

void GUIAnimationText::RemoveNextAnimation()
{
	if (m_nextAnimation) {
		m_nextAnimation.reset();
	}
	m_nextText = "";

	GUIAnimation::RemoveNextAnimation();
}

void GUIAnimationText::Update()
{
	if (!m_uiElement) {
		return;
	}

	//テキストの更新
	m_uiElement->m_size = m_size;

	GUIAnimation::Update();
}

void GUIAnimationText::SetNextAnimation()
{
	UIText* uiText = static_cast<UIText*>(m_uiElement.get());
	uiText->m_text = m_nextText;
	m_size = XMFLOAT2(m_nextScale, m_nextScale);
}

//----------GUIAnimationTexture----------

GUIAnimationTexture::GUIAnimationTexture()
	: m_nextAnimation(nullptr)
{
	m_size = XMFLOAT2(1.0f, 1.0f);
}

void GUIAnimationTexture::SetNextAnimation(UITextureRef nextTexture, float continueTime)
{
	std::shared_ptr<GUIAnimationTexture> nextAnim = std::make_shared<GUIAnimationTexture>();
	nextAnim->m_continueTime = continueTime;
	nextAnim->m_uiElement = nextTexture;

	m_nextAnimation = nextAnim;
	m_nextAnimationTemp = m_nextAnimation.get();
}

void GUIAnimationTexture::RemoveNextAnimation()
{
	if (m_nextAnimation) {
		m_nextAnimation.reset();
	}
	GUIAnimation::RemoveNextAnimation();
}

void GUIAnimationTexture::Update()
{
	if (!m_uiElement) {
		return;
	}

	//画像の更新
	UITexture* pUITexture = static_cast<UITexture*>(m_uiElement.get());
	pUITexture->m_size = pUITexture->GetTextureSize() * m_size;

	GUIAnimation::Update();
}

void GUIAnimationTexture::SetNextAnimation()
{
	m_size = XMFLOAT2(m_nextScale, m_nextScale);
}
