#include "SceneGame.h"

using namespace DirectX;

const std::string SceneGame::FONT_MESSAGE = "Resource\\Fonts\\LightNovelPOPv2.otf";

SceneGame::SceneGame()
	: m_sceneTime(0.0f)
{
}

SceneGame::~SceneGame()
{
}

void SceneGame::Start()
{
	//視野角を設定
	g_Engine->GetCamera()->SetFov(85.0f);

	//ウィンドウモードを最大化
	g_Engine->SetWindowMode(WindowMode::WindowMaximum, nullptr);

	//環境音の再生
	if (!g_Engine->GetWwiseSoundSystem()->IsInited()) {
		g_Engine->GetWwiseSoundSystem()->Initialize(KeyString::SOUND_INIT_BNK);
		g_Engine->GetWwiseSoundSystem()->Load_Bank(KeyString::SOUND_BGM_BNK);
		g_Engine->GetWwiseSoundSystem()->Load_Bank(KeyString::SOUND_SE_BNK);
	}
	g_Engine->GetWwiseSoundSystem()->Play(KeyString::SOUND_BGM_GAME);


	/*m_hairPhysics.Initialize(m_physX.GetPhysics(), m_physX.GetScene());
	m_hairPhysics.SetBone(m_pChar1->GetBoneManager(), m_pChar1->GetBone("Hair Root"));*/

	//各3Dモデルの初期化
	m_terrain.Initialize();
	m_character.Initialize();

	XMFLOAT2 windowSize = XMFLOAT2(static_cast<float>(g_Engine->GetWindowSize().cx), static_cast<float>(g_Engine->GetWindowSize().cy));

	//黒背景
	m_uiBlack = g_Engine->GetUIManager()->AddUITexture(Color::BLACK);
	m_uiBlack->m_size = windowSize;
	m_uiBlack->m_position = XMFLOAT2(windowSize.x / 2.0f, windowSize.y / 2.0f);

	//画面下の文字
	m_uiText = g_Engine->GetUIManager()->AddUIText(FONT_MESSAGE, 80);
	m_uiText->m_text = "-これはDirectX12のデモプレイです-";
	m_uiText->m_position = XMFLOAT2(windowSize.x / 2.0f, windowSize.y - windowSize.y / 10.0f);
	m_uiText->m_color = Color::CYAN;
	m_uiText->SetTextDistance(XMFLOAT2(10.0f, 10.0f));

	//画面左上の文字
	m_uiHelpText = g_Engine->GetUIManager()->AddUIText(FONT_MESSAGE, 40);
	m_uiHelpText->m_text = "-操作説明-\nカメラ移動 : WASD\n";
	m_uiHelpText->m_text += "カメラ上昇/下降 : スペース / 左シフト\n";
	m_uiHelpText->m_text += "カメラスピード上昇 : 左コントロール長押し\n";
	m_uiHelpText->m_text += "アニメーション再生/停止 : 左クリック";
	m_uiHelpText->m_position = XMFLOAT2(10.0f, 20.0f);
	m_uiHelpText->m_color = Color::BLACK;
	m_uiHelpText->m_bCenterPosition = false;

	printf("シーンの初期化に成功\n");
}

void SceneGame::Update()
{
	m_sceneTime += g_Engine->GetFrameTime();

	//カメラを更新
	UpdateCamera();

	//キー入力イベントを更新
	UpdateKeys();

	//各ボーンに球体を設置 (未使用)
	for (BoneSphere& bs : m_spheres) {
		XMMATRIX test = bs.pBone->GetGlobalTransform();
		bs.pModel->m_position.x = test.r[3].m128_f32[0];
		bs.pModel->m_position.y = test.r[3].m128_f32[1];
		bs.pModel->m_position.z = test.r[3].m128_f32[2];
	}

	//地形を更新 (ヒューマノイドモデル以外の3Dモデル)
	m_terrain.Update();

	//Wwiseのリスナーをカメラに設定
	Camera* pCamera = g_Engine->GetCamera();
	XMFLOAT3 eyePos, eyeFront, eyeTop;
	XMStoreFloat3(&eyePos, pCamera->m_eyePos);
	XMStoreFloat3(&eyeFront, pCamera->m_direction);
	//カメラの右方向ベクトルを計算
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(pCamera->m_upFoward, pCamera->m_direction));
	//Top = cross(Right, Front)
	XMVECTOR camUp = XMVector3Cross(right, pCamera->m_direction);
	XMStoreFloat3(&eyeTop, camUp);
	g_Engine->GetWwiseSoundSystem()->Set_Listener_Position(eyePos, eyeFront, eyeTop);
	g_Engine->GetWwiseSoundSystem()->Update();

	//黒背景を徐々に透明化
	if (m_sceneTime > 1.0f && m_uiBlack->m_color.a > 0.0f) {
		m_uiBlack->m_color.a -= g_Engine->GetFrameTime();
	}

	//ヒューマノイドモデルを更新
	m_character.Update();

	/*
	m_physX.Update(g_Engine->GetFrameTime());

	//m_hairPhysics.Update();

	//m_pDynamicBone->Update();
	*/
	//printf("MousePosX = %d, MousePosY = %d\n", g_Engine->GetMousePosition().x, g_Engine->GetMousePosition().y);
}

void SceneGame::Draw()
{
	//2D描画のみ行う

	m_uiText->Draw();
	m_uiHelpText->Draw();

	if (m_uiBlack->m_color.a > 0.0f) {
		m_uiBlack->Draw();
	}
}

void SceneGame::UpdateCamera()
{
	Camera* pCamera = g_Engine->GetCamera();

	//カメラの前方ベクトルを計算
	XMVECTOR forward = XMVector3Normalize(pCamera->m_targetPos - pCamera->m_eyePos);

	//カメラの右方向ベクトルを計算
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(pCamera->m_upFoward, forward));

	float cameraSpeed = 1.2f * g_Engine->GetFrameTime();
	float rotationSpeed = 0.1f * g_Engine->GetFrameTime();

	if (g_Engine->GetKeyState(DIK_LCONTROL)) {
		cameraSpeed *= 4.0f;
	}

	//上昇
	if (g_Engine->GetKeyState(DIK_SPACE)) {
		pCamera->m_eyePos = XMVectorAdd(pCamera->m_eyePos, XMVectorScale(pCamera->m_upFoward, cameraSpeed));
		pCamera->m_targetPos = XMVectorAdd(pCamera->m_targetPos, XMVectorScale(pCamera->m_upFoward, cameraSpeed));
	}
	//下降
	else if (g_Engine->GetKeyState(DIK_LSHIFT)) {
		pCamera->m_eyePos = XMVectorSubtract(pCamera->m_eyePos, XMVectorScale(pCamera->m_upFoward, cameraSpeed));
		pCamera->m_targetPos = XMVectorSubtract(pCamera->m_targetPos, XMVectorScale(pCamera->m_upFoward, cameraSpeed));
	}

	//右移動、左移動
	if (g_Engine->GetKeyState(DIK_A)) {
		pCamera->m_eyePos = XMVectorAdd(pCamera->m_eyePos, XMVectorScale(right, cameraSpeed));
		pCamera->m_targetPos = XMVectorAdd(pCamera->m_targetPos, XMVectorScale(right, cameraSpeed));
	}
	else if (g_Engine->GetKeyState(DIK_D)) {
		pCamera->m_eyePos = XMVectorSubtract(pCamera->m_eyePos, XMVectorScale(right, cameraSpeed));
		pCamera->m_targetPos = XMVectorSubtract(pCamera->m_targetPos, XMVectorScale(right, cameraSpeed));
	}

	//前進、後進
	if (g_Engine->GetKeyState(DIK_S)) {
		pCamera->m_eyePos = XMVectorSubtract(pCamera->m_eyePos, XMVectorScale(forward, cameraSpeed));
		pCamera->m_targetPos = XMVectorSubtract(pCamera->m_targetPos, XMVectorScale(forward, cameraSpeed));
	}
	else if (g_Engine->GetKeyState(DIK_W)) {
		pCamera->m_eyePos = XMVectorAdd(pCamera->m_eyePos, XMVectorScale(forward, cameraSpeed));
		pCamera->m_targetPos = XMVectorAdd(pCamera->m_targetPos, XMVectorScale(forward, cameraSpeed));
	}

	//マウス移動量による視点変更
	POINT mouseDelta = g_Engine->GetMouseMove();

	//yawとpitchを更新
	pCamera->m_yaw -= mouseDelta.x * rotationSpeed;
	pCamera->m_pitch -= mouseDelta.y * rotationSpeed;

	//垂直回転角度の制限
	const float pitchLimit = XM_PIDIV2 - 0.01f;
	pCamera->m_pitch = std::clamp(pCamera->m_pitch, -pitchLimit, pitchLimit);

	//カメラの回転を計算
	XMVECTOR rotation = XMQuaternionRotationRollPitchYaw(pCamera->m_pitch, pCamera->m_yaw, 0.0f);
	XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rotation);

	//カメラの方向ベクトルを更新
	XMVECTOR defaultForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMVECTOR newForward = XMVector3TransformNormal(defaultForward, rotationMatrix);

	pCamera->m_targetPos = pCamera->m_eyePos + newForward;
}

void SceneGame::UpdateKeys()
{
	//N または Mキーで環境光の角度を変更
	if (g_Engine->GetKeyState(DIK_N)) {
		g_Engine->GetDirectionalLight()->AddRotationX(-0.5f);
	}
	if (g_Engine->GetKeyState(DIK_M)) {
		g_Engine->GetDirectionalLight()->AddRotationX(0.5f);
	}
}
