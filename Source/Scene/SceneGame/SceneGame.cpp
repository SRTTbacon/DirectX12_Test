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
	//����p��ݒ�
	g_Engine->GetCamera()->SetFov(85.0f);

	//�E�B���h�E���[�h���ő剻
	g_Engine->SetWindowMode(WindowMode::WindowMaximum, nullptr);

	//�����̍Đ�
	if (!g_Engine->GetWwiseSoundSystem()->IsInited()) {
		g_Engine->GetWwiseSoundSystem()->Initialize(KeyString::SOUND_INIT_BNK);
		g_Engine->GetWwiseSoundSystem()->Load_Bank(KeyString::SOUND_BGM_BNK);
		g_Engine->GetWwiseSoundSystem()->Load_Bank(KeyString::SOUND_SE_BNK);
	}
	g_Engine->GetWwiseSoundSystem()->Play(KeyString::SOUND_BGM_GAME);


	/*m_hairPhysics.Initialize(m_physX.GetPhysics(), m_physX.GetScene());
	m_hairPhysics.SetBone(m_pChar1->GetBoneManager(), m_pChar1->GetBone("Hair Root"));*/

	//�e3D���f���̏�����
	m_terrain.Initialize();
	m_character.Initialize();

	XMFLOAT2 windowSize = XMFLOAT2(static_cast<float>(g_Engine->GetWindowSize().cx), static_cast<float>(g_Engine->GetWindowSize().cy));

	//���w�i
	m_uiBlack = g_Engine->GetUIManager()->AddUITexture(Color::BLACK);
	m_uiBlack->m_size = windowSize;
	m_uiBlack->m_position = XMFLOAT2(windowSize.x / 2.0f, windowSize.y / 2.0f);

	//��ʉ��̕���
	m_uiText = g_Engine->GetUIManager()->AddUIText(FONT_MESSAGE, 80);
	m_uiText->m_text = "-�����DirectX12�̃f���v���C�ł�-";
	m_uiText->m_position = XMFLOAT2(windowSize.x / 2.0f, windowSize.y - windowSize.y / 10.0f);
	m_uiText->m_color = Color::CYAN;
	m_uiText->SetTextDistance(XMFLOAT2(10.0f, 10.0f));

	//��ʍ���̕���
	m_uiHelpText = g_Engine->GetUIManager()->AddUIText(FONT_MESSAGE, 40);
	m_uiHelpText->m_text = "-�������-\n�J�����ړ� : WASD\n";
	m_uiHelpText->m_text += "�J�����㏸/���~ : �X�y�[�X / ���V�t�g\n";
	m_uiHelpText->m_text += "�J�����X�s�[�h�㏸ : ���R���g���[��������\n";
	m_uiHelpText->m_text += "�A�j���[�V�����Đ�/��~ : ���N���b�N";
	m_uiHelpText->m_position = XMFLOAT2(10.0f, 20.0f);
	m_uiHelpText->m_color = Color::BLACK;
	m_uiHelpText->m_bCenterPosition = false;

	printf("�V�[���̏������ɐ���\n");
}

void SceneGame::Update()
{
	m_sceneTime += g_Engine->GetFrameTime();

	//�J�������X�V
	UpdateCamera();

	//�L�[���̓C�x���g���X�V
	UpdateKeys();

	//�e�{�[���ɋ��̂�ݒu (���g�p)
	for (BoneSphere& bs : m_spheres) {
		XMMATRIX test = bs.pBone->GetGlobalTransform();
		bs.pModel->m_position.x = test.r[3].m128_f32[0];
		bs.pModel->m_position.y = test.r[3].m128_f32[1];
		bs.pModel->m_position.z = test.r[3].m128_f32[2];
	}

	//�n�`���X�V (�q���[�}�m�C�h���f���ȊO��3D���f��)
	m_terrain.Update();

	//Wwise�̃��X�i�[���J�����ɐݒ�
	Camera* pCamera = g_Engine->GetCamera();
	XMFLOAT3 eyePos, eyeFront, eyeTop;
	XMStoreFloat3(&eyePos, pCamera->m_eyePos);
	XMStoreFloat3(&eyeFront, pCamera->m_direction);
	//�J�����̉E�����x�N�g�����v�Z
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(pCamera->m_upFoward, pCamera->m_direction));
	//Top = cross(Right, Front)
	XMVECTOR camUp = XMVector3Cross(right, pCamera->m_direction);
	XMStoreFloat3(&eyeTop, camUp);
	g_Engine->GetWwiseSoundSystem()->Set_Listener_Position(eyePos, eyeFront, eyeTop);
	g_Engine->GetWwiseSoundSystem()->Update();

	//���w�i�����X�ɓ�����
	if (m_sceneTime > 1.0f && m_uiBlack->m_color.a > 0.0f) {
		m_uiBlack->m_color.a -= g_Engine->GetFrameTime();
	}

	//�q���[�}�m�C�h���f�����X�V
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
	//2D�`��̂ݍs��

	m_uiText->Draw();
	m_uiHelpText->Draw();

	if (m_uiBlack->m_color.a > 0.0f) {
		m_uiBlack->Draw();
	}
}

void SceneGame::UpdateCamera()
{
	Camera* pCamera = g_Engine->GetCamera();

	//�J�����̑O���x�N�g�����v�Z
	XMVECTOR forward = XMVector3Normalize(pCamera->m_targetPos - pCamera->m_eyePos);

	//�J�����̉E�����x�N�g�����v�Z
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(pCamera->m_upFoward, forward));

	float cameraSpeed = 1.2f * g_Engine->GetFrameTime();
	float rotationSpeed = 0.1f * g_Engine->GetFrameTime();

	if (g_Engine->GetKeyState(DIK_LCONTROL)) {
		cameraSpeed *= 4.0f;
	}

	//�㏸
	if (g_Engine->GetKeyState(DIK_SPACE)) {
		pCamera->m_eyePos = XMVectorAdd(pCamera->m_eyePos, XMVectorScale(pCamera->m_upFoward, cameraSpeed));
		pCamera->m_targetPos = XMVectorAdd(pCamera->m_targetPos, XMVectorScale(pCamera->m_upFoward, cameraSpeed));
	}
	//���~
	else if (g_Engine->GetKeyState(DIK_LSHIFT)) {
		pCamera->m_eyePos = XMVectorSubtract(pCamera->m_eyePos, XMVectorScale(pCamera->m_upFoward, cameraSpeed));
		pCamera->m_targetPos = XMVectorSubtract(pCamera->m_targetPos, XMVectorScale(pCamera->m_upFoward, cameraSpeed));
	}

	//�E�ړ��A���ړ�
	if (g_Engine->GetKeyState(DIK_A)) {
		pCamera->m_eyePos = XMVectorAdd(pCamera->m_eyePos, XMVectorScale(right, cameraSpeed));
		pCamera->m_targetPos = XMVectorAdd(pCamera->m_targetPos, XMVectorScale(right, cameraSpeed));
	}
	else if (g_Engine->GetKeyState(DIK_D)) {
		pCamera->m_eyePos = XMVectorSubtract(pCamera->m_eyePos, XMVectorScale(right, cameraSpeed));
		pCamera->m_targetPos = XMVectorSubtract(pCamera->m_targetPos, XMVectorScale(right, cameraSpeed));
	}

	//�O�i�A��i
	if (g_Engine->GetKeyState(DIK_S)) {
		pCamera->m_eyePos = XMVectorSubtract(pCamera->m_eyePos, XMVectorScale(forward, cameraSpeed));
		pCamera->m_targetPos = XMVectorSubtract(pCamera->m_targetPos, XMVectorScale(forward, cameraSpeed));
	}
	else if (g_Engine->GetKeyState(DIK_W)) {
		pCamera->m_eyePos = XMVectorAdd(pCamera->m_eyePos, XMVectorScale(forward, cameraSpeed));
		pCamera->m_targetPos = XMVectorAdd(pCamera->m_targetPos, XMVectorScale(forward, cameraSpeed));
	}

	//�}�E�X�ړ��ʂɂ�鎋�_�ύX
	POINT mouseDelta = g_Engine->GetMouseMove();

	//yaw��pitch���X�V
	pCamera->m_yaw -= mouseDelta.x * rotationSpeed;
	pCamera->m_pitch -= mouseDelta.y * rotationSpeed;

	//������]�p�x�̐���
	const float pitchLimit = XM_PIDIV2 - 0.01f;
	pCamera->m_pitch = std::clamp(pCamera->m_pitch, -pitchLimit, pitchLimit);

	//�J�����̉�]���v�Z
	XMVECTOR rotation = XMQuaternionRotationRollPitchYaw(pCamera->m_pitch, pCamera->m_yaw, 0.0f);
	XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rotation);

	//�J�����̕����x�N�g�����X�V
	XMVECTOR defaultForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMVECTOR newForward = XMVector3TransformNormal(defaultForward, rotationMatrix);

	pCamera->m_targetPos = pCamera->m_eyePos + newForward;
}

void SceneGame::UpdateKeys()
{
	//N �܂��� M�L�[�Ŋ����̊p�x��ύX
	if (g_Engine->GetKeyState(DIK_N)) {
		g_Engine->GetDirectionalLight()->AddRotationX(-0.5f);
	}
	if (g_Engine->GetKeyState(DIK_M)) {
		g_Engine->GetDirectionalLight()->AddRotationX(0.5f);
	}
}
