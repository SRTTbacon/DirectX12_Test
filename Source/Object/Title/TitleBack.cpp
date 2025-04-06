#include "TitleBack.h"

#include "..\\..\\System\\Engine\\Model\\Convert\\ConvertFromFBX.h"

const std::string TitleBack::MODEL_CASTLE = "Resource\\Model\\Castle.hcs";
const std::string TitleBack::TEXTURE_CASTLE = "Resource\\Model\\Castle.png";
const std::string TitleBack::TEXTURE_SKYBOX = "Resource\\Texture\\SkyBox.dds";

TitleBack::TitleBack()
	: m_bCameraMove(true)
	, m_pCastle(nullptr)
{
}

TitleBack::~TitleBack()
{
	if (m_pCastle) {
		g_Engine->ReleaseModel(m_pCastle);
		m_pCastle = nullptr;
	}
}

void TitleBack::Initialize()
{
	g_Engine->GetSkyBox()->SetSkyTexture(TEXTURE_SKYBOX);

	g_Engine->GetWwiseSoundSystem()->Initialize(KeyString::SOUND_INIT_BNK);
	g_Engine->GetWwiseSoundSystem()->Load_Bank(KeyString::SOUND_BGM_BNK);
	g_Engine->GetWwiseSoundSystem()->Load_Bank(KeyString::SOUND_SE_BNK);
	g_Engine->GetWwiseSoundSystem()->Play(KeyString::SOUND_BGM_TITLE);

	m_pCastle = g_Engine->AddModel(MODEL_CASTLE);
	m_pCastle->m_position = XMFLOAT3(0.0f, 2.0f, 2.0f);
	m_pCastle->m_scale = XMFLOAT3(0.01f, 0.01f, 0.01f);
	m_pCastle->SetRotation(180.0f, 0.0f, 0.0f);
	m_pCastle->GetMesh(0)->pMaterial = g_Engine->GetMaterialManager()->AddMaterial("Castle");
	m_pCastle->GetMesh(0)->pMaterial->SetMainTexture(TEXTURE_CASTLE);
}

void TitleBack::Update()
{
	if (!m_bCameraMove) {
		return;
	}

	Camera* pCamera = g_Engine->GetCamera();

	//カメラの前方ベクトルを計算
	XMVECTOR forward = XMVector3Normalize(pCamera->m_targetPos - pCamera->m_eyePos);

	//カメラの右方向ベクトルを計算
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(pCamera->m_upFoward, forward));

	float rotationSpeed = 0.025f * g_Engine->GetFrameTime();

	pCamera->m_yaw -= rotationSpeed;

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

void TitleBack::SetMoveCamera(bool bMove)
{
	m_bCameraMove = bMove;
}
