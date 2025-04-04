#pragma once

#include "..\\..\\GameBase.h"

class GUIElement
{
public:
	DirectX::XMFLOAT2 m_position;
	DirectX::XMFLOAT2 m_size;

	GUIElement();
};

class GUIAnimation : public GUIElement
{
public:
	GUIAnimation();

	void Initialize(UIElementRef uiElement);

	void StartAnimation();

	virtual void RemoveNextAnimation();
	virtual void Update();
	void Draw();

	float m_fadeInTime;		//フェードインにかかる時間
	float m_fadeOutTime;	//フェードアウトにかかる時間
	float m_intervalTime;	//フェードアウトまでの時間
	float m_continueTime;	//次のアニメーションまでの時間 (次があれば)

	float m_nextScale;
	float m_animTime;

protected:
	virtual void SetNextAnimation() = 0;

	UIElementRef m_uiElement;

	GUIAnimation* m_nextAnimationTemp;
};

class GUIAnimationText : public GUIAnimation
{
public:
	GUIAnimationText();

	void SetNextAnimation(std::string text, float scale = 1.0f, float continueTime = 0.0f);

	void RemoveNextAnimation();

	void Update();

public:
	inline GUIAnimationText* GetNextAnimation() const { return m_nextAnimation.get(); }
	inline UIText* GetUIText() const { return static_cast<UIText*>(m_uiElement.get()); }

protected:
	void SetNextAnimation();

private:
	std::shared_ptr<GUIAnimationText> m_nextAnimation;
	std::string m_nextText;
};

class GUIAnimationTexture : public GUIAnimation
{
public:
	GUIAnimationTexture();

	void SetNextAnimation(UITextureRef nextTexture, float continueTime = 0.0f);

	void RemoveNextAnimation();

	void Update();

public:
	inline GUIAnimationTexture* GetNextAnimation() const { return m_nextAnimation.get(); }
	inline UITexture* GetUIText() const { return static_cast<UITexture*>(m_uiElement.get()); }

protected:
	void SetNextAnimation();

private:
	std::shared_ptr<GUIAnimationTexture> m_nextAnimation;
};
