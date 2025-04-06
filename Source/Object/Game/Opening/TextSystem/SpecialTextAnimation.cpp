#include "SpecialTextAnimation.h"
#include "..\\..\\..\\..\\System\\Engine\\Core\\Color.h"

SpecialTextAnimation::SpecialTextAnimation()
	: m_firstBackColor(Color::WHITE)
	, m_firstTextColor(Color::BLACK)
	, m_secondBackColor(Color::RED)
	, m_secondTextColor(Color::BLUE)
	, m_endBackColor(Color::BLACK)
	, m_endTextColor(Color::WHITE)
	, m_startIntervalTime(0.0f)
	, m_firstIntervalTime(0.1f)
	, m_secondIntervalTime(0.05f)
	, m_animationTime(-1.0f)
	, m_endTime(5.0f)
	, m_bAnimation(false)
	, m_bEnableSecondColor(false)
{
}

void SpecialTextAnimation::Initialize(std::string strFontPath, UINT fontSize, std::string text)
{
	m_whiteTexture = g_Engine->GetUIManager()->AddUITexture(Color::WHITE);
	m_text = g_Engine->GetUIManager()->AddUIText(strFontPath, fontSize);

	m_whiteTexture->m_bCenterPosition = false;

	SIZE windowSize = g_Engine->GetWindowSize();
	m_whiteTexture->m_size = XMFLOAT2(static_cast<float>(windowSize.cx), static_cast<float>(windowSize.cy));

	m_text->m_position = XMFLOAT2(windowSize.cx / 2.0f, windowSize.cy / 2.0f);

	m_text->m_text = text;
}

void SpecialTextAnimation::SetNextAnimation(std::string strFontPath, UINT fontSize, std::string text)
{
	m_pNextTextAnimation = std::make_shared<SpecialTextAnimation>();
	m_pNextTextAnimation->Initialize(strFontPath, fontSize, text);
}

void SpecialTextAnimation::StartAnimation()
{
	m_animationTime = 0.0f;
}

void SpecialTextAnimation::StopAnimation()
{
	m_bAnimation = false;
	m_animationTime = -1.0f;
}

void SpecialTextAnimation::Update()
{
	if (m_animationTime < 0.0f) {
		return;
	}

	if (m_animationTime >= 0.0f) {
		m_animationTime += g_Engine->GetFrameTime();
	}

	if (!m_bAnimation && m_animationTime >= m_startIntervalTime) {
		m_animationTime = 0.0f;
		m_bAnimation = true;
	}

	if (m_bAnimation) {

		// シェイクの周波数 (ランダムな振動)
		float frequency = 10.0f;
		float timeFactor = m_animationTime * frequency;

		//自然な揺れ (サイン波ベース)
		float offsetX = std::sin(timeFactor) * 0.035f;
		float offsetY = std::cos(timeFactor * 1.3f) * 0.035f;

		m_text->m_position.x += offsetX;
		m_text->m_position.y += offsetY;

		if (m_animationTime >= m_endTime) {
			ResetToNextAnimation();
		}
	}
}

void SpecialTextAnimation::Draw()
{
	if (!m_bAnimation) {
		return;
	}

	if (m_animationTime < m_firstIntervalTime) {
		m_whiteTexture->m_color = m_firstBackColor;
		m_text->m_color = m_firstTextColor;
	}
	else if (m_bEnableSecondColor && m_animationTime < m_secondIntervalTime + m_firstIntervalTime) {
		m_whiteTexture->m_color = m_secondBackColor;
		m_text->m_color = m_secondTextColor;
	}
	else {
		m_whiteTexture->m_color = m_endBackColor;
		m_text->m_color = m_endTextColor;
	}

	m_whiteTexture->Draw();

	m_text->Draw();
}

void SpecialTextAnimation::ResetToNextAnimation()
{
	if (!m_pNextTextAnimation) {
		m_animationTime = -1.0f;
		m_bAnimation = false;
		return;
	}

	m_animationTime = 0.0f;
	m_text = m_pNextTextAnimation->m_text;

	m_firstIntervalTime = m_pNextTextAnimation->m_firstIntervalTime;
	m_firstBackColor = m_pNextTextAnimation->m_firstBackColor;
	m_firstTextColor = m_pNextTextAnimation->m_firstTextColor;

	m_secondIntervalTime = m_pNextTextAnimation->m_secondIntervalTime;
	m_secondBackColor = m_pNextTextAnimation->m_secondBackColor;
	m_secondTextColor = m_pNextTextAnimation->m_secondTextColor;

	m_endBackColor = m_pNextTextAnimation->m_endBackColor;
	m_endTextColor = m_pNextTextAnimation->m_endTextColor;

	m_startIntervalTime = m_pNextTextAnimation->m_startIntervalTime;
	m_endTime = m_pNextTextAnimation->m_endTime;

	m_bEnableSecondColor = m_pNextTextAnimation->m_bEnableSecondColor;
	m_bAnimation = false;

	m_pNextTextAnimation = m_pNextTextAnimation->GetNextAnimation();
}
