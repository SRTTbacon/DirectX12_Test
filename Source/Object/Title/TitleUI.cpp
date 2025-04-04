#include "TitleUI.h"

using namespace KeyString;

const char* TitleUI::TITLE_TEXTURE_PATH = "Resource\\Texture\\Title.hcs";
const char* TitleUI::BUTTON_EXIT_01 = "Resource\\Texture\\Title\\Button_Escape_01.png";
const char* TitleUI::BUTTON_EXIT_02 = "Resource\\Texture\\Title\\Button_Escape_02.png";
const char* TitleUI::TEXTURE_WWISE_ICON = "Resource\\Texture\\Title\\Wwise.png";
const char* TitleUI::BLACKOUT_HCS_PATH = "Resource\\Effects\\Blackout.hcs";
const char* TitleUI::BLACKOUT_PATH = "Resource\\Effects\\Blackout\\";
const float TitleUI::FRIEZE_TIME = 4.0f;
const float TitleUI::FRIEZE_NOISE_START_TIME = 1.0f;
const float TitleUI::OPENING_TIME = 9.0f;
const float TitleUI::BLACKOUT_FRAME_TIME = 0.0275f;
const float TitleUI::BLACKOUT_EXIT_TIME = 0.5f;

TitleUI::TitleUI(SceneData* pSceneData)
	: m_nowBlackOutFrame(-1)
	, m_sceneTime(0.0f)
	, m_friezeTime(-1.0f)
	, m_frameTimeBlackOut(0.0f)
	, m_frameTimeBlackOutExit(0.0f)
	, m_pSceneData(pSceneData)
	, m_gameStarting(false)
	, m_bCanOperation(false)
	, m_bStartedOpening(false)
{
}

TitleUI::~TitleUI()
{
	for (UINT i = 0; i < m_blackOuts.size(); i++) {
		delete m_blackOuts[i];
	}

	m_blackOuts.clear();
}

void TitleUI::Initilaize()
{
	XMFLOAT2 windowSize = XMFLOAT2(static_cast<float>(g_Engine->GetWindowSize().cx), static_cast<float>(g_Engine->GetWindowSize().cy));

	//ブラックアウト用のテクスチャ
	m_uiBlackOutTexture = g_Engine->GetUIManager()->AddUITexture(Color::BLACK);
	m_uiBlackOutTexture->m_size = windowSize;

	HCSReadFile titleUITexture;
	titleUITexture.Open(TITLE_TEXTURE_PATH);

	//終了ボタン
	m_exitButton = GUILayout::CreateButton(titleUITexture, BUTTON_EXIT_01, BUTTON_EXIT_02, XMFLOAT2(200, 200));
	m_exitButton.m_position = XMFLOAT2(960.0f, 540.0f);

	//黒背景
	m_uiBlackBack = g_Engine->GetUIManager()->AddUITexture(Color::BLACK);
	m_uiBlackBack->m_size = windowSize;
	m_uiBlackBack->m_color.a = 1.0f;

	//タイトルテキスト
	m_uiTextEngine = GUILayout::CreateAnimationText(FONT_NOTOSERIF, 75, "ALICE ENGINE");
	m_uiTextEngine.m_size = XMFLOAT2(1.5f, 1.5f);
	m_uiTextEngine.m_position = XMFLOAT2(windowSize.x / 2.0f, windowSize.y / 3.0f);
	m_uiTextEngine.GetUIText()->SetTextDistance(XMFLOAT2(10.0f, 10.0f));
	m_uiTextEngine.SetNextAnimation("このゲームはDirectX12を使用した\n自作のエンジンを使用しています", 1.0f);
	m_uiTextEngine.GetNextAnimation()->m_intervalTime = 3.0f;
	m_uiTextEngine.StartAnimation();

	m_uiTextureEngine = GUILayout::CreateAnimationTexture(titleUITexture, TEXTURE_WWISE_ICON);
	m_uiTextureEngine.m_size = XMFLOAT2(1.0f, 1.0f);
	m_uiTextureEngine.m_position = XMFLOAT2(windowSize.x / 2.0f, windowSize.y - windowSize.y / 4.0f);
	m_uiTextureEngine.m_intervalTime = 7.0f;
	m_uiTextureEngine.StartAnimation();

	//ブラックアウトをロード
	HCSReadFile hcsReadFile;
	hcsReadFile.Open(BLACKOUT_HCS_PATH);
	for (UINT i = 0; i < hcsReadFile.GetFileCount(); i++) {
		size_t size;
		char* pBuffer = hcsReadFile.GetFile(BLACKOUT_PATH + std::to_string(i) + ".png", &size);
		//Texture2D* pTexture = Texture2D::Get(BLACKOUT_PATH + std::to_string(i) + ".png");
		Texture2D* pTexture = Texture2D::Get(pBuffer, size);
		m_blackOuts.push_back(pTexture);

		delete pBuffer;
	}

	m_openingTextAnimation.Initialize(FONT_NOTOSERIF, 150, "制作 : 竹内 翼");
	m_openingTextAnimation.m_text->SetTextDistance(XMFLOAT2(10.0f, 10.0f));
	m_openingTextAnimation.m_endTime = 3.0f;
	m_openingTextAnimation.SetNextAnimation(FONT_NOTOSERIF, 150, "2D、3Dのデモプレイです");
	m_openingTextAnimation.GetNextAnimation()->m_text->SetTextDistance(XMFLOAT2(10.0f, 10.0f));
	m_openingTextAnimation.GetNextAnimation()->m_endTime = 3.0f;
	m_openingTextAnimation.GetNextAnimation()->m_bEnableSecondColor = true;
	m_openingTextAnimation.GetNextAnimation()->m_text->m_size = XMFLOAT2(0.75f, 0.75f);
	m_openingTextAnimation.GetNextAnimation()->SetNextAnimation(FONT_NOTOSERIF, 150, "");
	m_openingTextAnimation.GetNextAnimation()->GetNextAnimation()->m_endTime = 3.0f;
	m_openingTextAnimation.GetNextAnimation()->GetNextAnimation()->m_bEnableSecondColor = true;
}

void TitleUI::Update()
{
	if (g_Engine->GetKeyStateSync(DIK_C)) {
		m_nowBlackOutFrame = 0;
		g_Engine->GetWwiseSoundSystem()->Play(SOUND_SE_TITLE_BLACKOUT);
		m_bStartedOpening = false;
		m_friezeTime = -1.0f;
		m_frameTimeBlackOut = 0.0f;
		m_frameTimeBlackOutExit = 0.0f;
		m_openingTextAnimation.StopAnimation();
		m_openingTextAnimation.Initialize(FONT_NOTOSERIF, 150, "まさか、");
		m_openingTextAnimation.m_bEnableSecondColor = false;
		m_openingTextAnimation.m_text->SetTextDistance(XMFLOAT2(10.0f, 10.0f));
		m_openingTextAnimation.m_endTime = 3.0f;
		m_openingTextAnimation.SetNextAnimation(FONT_NOTOSERIF, 150, "暴走！？");
		m_openingTextAnimation.GetNextAnimation()->m_text->SetTextDistance(XMFLOAT2(10.0f, 10.0f));
		m_openingTextAnimation.GetNextAnimation()->m_endTime = 3.0f;
		m_openingTextAnimation.GetNextAnimation()->m_bEnableSecondColor = true;
		m_openingTextAnimation.GetNextAnimation()->SetNextAnimation(FONT_NOTOSERIF, 150, "");
		m_openingTextAnimation.GetNextAnimation()->GetNextAnimation()->m_endTime = 3.0f;
		m_openingTextAnimation.GetNextAnimation()->GetNextAnimation()->m_bEnableSecondColor = true;
	}

	m_sceneTime += g_Engine->GetFrameTime();

	UpdateBlackBack();
	UpdateFrieze();
	UpdataBlackOut();

	m_openingTextAnimation.Update();

	m_uiTextEngine.Update();
	m_uiTextureEngine.Update();

	if (m_bCanOperation && m_exitButton.Update()) {
		m_bCanOperation = false;
		m_friezeTime = 0.0f;
		m_gameStarting = true;
	}
}

void TitleUI::Draw()
{
	m_exitButton.Draw();

	m_uiBlackBack->Draw();

	if (m_nowBlackOutFrame >= 0) {
		m_uiBlackOutTexture->Draw();
	}

	m_openingTextAnimation.Draw();

	m_uiTextEngine.Draw();
	m_uiTextureEngine.Draw();
}

void TitleUI::UpdateBlackBack()
{
	if (!m_gameStarting && m_uiBlackBack->m_color.a > 0.0f && m_sceneTime >= OPENING_TIME) {
		m_uiBlackBack->m_color.a -= g_Engine->GetFrameTime();
		m_bCanOperation = true;
		if (m_uiBlackBack->m_color.a <= 0.0f) {
			m_uiBlackBack->m_color.a = 0.0f;
		}
	}
}

void TitleUI::UpdateFrieze()
{
	if (m_friezeTime >= 0.0f && m_nowBlackOutFrame < 0) {
		m_friezeTime += g_Engine->GetFrameTime();
		if (m_friezeTime >= FRIEZE_TIME) {
			g_Engine->SetEnablePostProcess(false);
			m_nowBlackOutFrame = 0;
			g_Engine->GetWwiseSoundSystem()->Stop_All();
			g_Engine->GetWwiseSoundSystem()->Play(SOUND_SE_TITLE_BLACKOUT);
		}
		else if (m_friezeTime >= FRIEZE_NOISE_START_TIME && !g_Engine->GetIsPostProcessEnabled()) {
			g_Engine->SetEnablePostProcess(true);
			g_Engine->GetWwiseSoundSystem()->Play(SOUND_SE_TITLE_NOISE);
		}
	}
}

void TitleUI::UpdataBlackOut()
{
	if (m_bStartedOpening) {
		return;
	}

	if (m_nowBlackOutFrame >= 0 && m_frameTimeBlackOutExit <= 0.0f) {
		m_frameTimeBlackOut += g_Engine->GetFrameTime();

		if (m_frameTimeBlackOut >= BLACKOUT_FRAME_TIME) {
			m_frameTimeBlackOut -= BLACKOUT_FRAME_TIME;
			m_nowBlackOutFrame++;
			m_uiBlackOutTexture->SetTexture(m_blackOuts[m_nowBlackOutFrame]);

			if (m_nowBlackOutFrame >= m_blackOuts.size() - 1) {
				m_frameTimeBlackOut = 0.0f;
				m_frameTimeBlackOutExit = g_Engine->GetFrameTime();
			}
		}
	}

	if (m_frameTimeBlackOutExit > 0.0f) {
		m_frameTimeBlackOutExit += g_Engine->GetFrameTime();
		if (m_frameTimeBlackOutExit >= BLACKOUT_EXIT_TIME) {
			//m_pSceneData->Set(Common::SCENE_EXIT);
			m_openingTextAnimation.StartAnimation();
			m_bStartedOpening = true;
		}
	}
}
