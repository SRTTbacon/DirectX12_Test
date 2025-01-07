#include "Scene.h"

Scene* g_Scene;

using namespace DirectX;

const std::string modelFile1 = "Resource/Model/FBX_Moe.hcs";
const std::string modelFile2 = "Resource/Model/Sphere.fbx";
const std::string modelFile3 = "Resource/Model/Plane.fbx";

bool bAnim = true;
bool bVisible = true;
float animSpeed = 0.95f;

bool Scene::Init()
{
	g_Engine->GetCamera()->SetFov(65.0f);

	//キャラクター
	for (int i = 0; i < 1; i++) {
		Character* pCharacter = g_Engine->AddCharacter(modelFile1);
		m_pModels.push_back(pCharacter);
		pCharacter->AddAnimation(g_Engine->GetAnimation("Resource\\Test5.hcs"));
		pCharacter->m_animationSpeed = animSpeed;

		pCharacter->SetShapeWeight("Big breasts.Big breasts", 1.0f);

		std::vector meshNames = { "Shirt", "Skirt", "Outer", "Brassiere", "Underwear"};
		for (const std::string& meshName : meshNames) {
			Character::HumanoidMesh* pMesh = pCharacter->GetHumanMesh(meshName);
			if (pMesh) {
				//pMesh->pMesh->bDraw = false;
			}
		}
	}

	//地面
	m_pModel2 = g_Engine->AddModel(modelFile3);
	m_pModel2->m_scale = XMFLOAT3(50.0f, 0.05f, 50.0f);
	m_pModel2->m_position.y = -0.05f;

	for (int i = 0; i < 1; i++) {
		m_pModels[i]->m_rotation.x = -90.0f;
		m_pModels[i]->m_rotation.y = 180.0f;
		if (i > 0) {
			m_pModels[i]->m_position.x = (float)((i % 2 == 0 ? 1.0f : -1.0f) * i / 2) + 1.0f;
		}
	}

	m_pBGMHandle = g_Engine->GetSoundSystem()->LoadSound("Resource\\BGM\\ビビデバDance.mp3", true);
	m_pBGMHandle->m_volume = 0.25f;
	m_pBGMHandle->m_speed = animSpeed;
	m_pBGMHandle->m_bLooping = true;
	m_pBGMHandle->UpdateProperty();

	//ConvertFromFBX convert;
	//convert.ConvertFromCharacter(m_pModels[0], "Resource/Model/FBX_Moe.fbx", "Resource/Model/FBX_Moe.hcs");

	printf("シーンの初期化に成功\n");
	return true;
}

void Scene::Update()
{
	UpdateCamera();

	if (g_Engine->GetMouseStateSync(0x00)) {
		bAnim = !bAnim;
		if (bAnim) {
			m_pModels[0]->m_animationSpeed = animSpeed;
			m_pBGMHandle->PlaySound(false);
		}
		else {
			m_pModels[0]->m_animationSpeed = 0.0f;
			m_pBGMHandle->PauseSound();
		}
	}
	if (g_Engine->GetMouseStateSync(0x01)) {
		m_pModels[0]->m_rotation.x += 90.0f;
	}
	if (g_Engine->GetMouseStateSync(0x02)) {
		m_pBGMHandle->SetPosition(m_pModels[0]->m_nowAnimationTime);
	}

	if (g_Engine->GetKeyStateSync(DIK_J)) {
		m_pModels[0]->m_nowAnimationTime -= 5.0f;
		if (m_pModels[0]->m_nowAnimationTime < 0.0f) {
			m_pModels[0]->m_nowAnimationTime = 0.0f;
		}

		m_pBGMHandle->SetPosition(m_pModels[0]->m_nowAnimationTime);
	}
	else if (g_Engine->GetKeyStateSync(DIK_K)) {
		m_pModels[0]->m_nowAnimationTime += 5.0f;
		m_pBGMHandle->SetPosition(m_pModels[0]->m_nowAnimationTime);
	}

	UINT shapeIndex = 331;
	if (g_Engine->GetKeyState(DIK_N)) {
		float weight = m_pModels[0]->GetShapeWeight("ウィンク");
		//float weight = m_pModels[0]->GetShapeWeight("Body", shapeIndex);
		weight -= g_Engine->GetFrameTime();
		m_pModels[0]->SetShapeWeight("ウィンク", weight);
		//m_pModels[0]->SetShapeWeight("Body", shapeIndex, weight);
	}
	if (g_Engine->GetKeyState(DIK_M)) {
		float weight = m_pModels[0]->GetShapeWeight("ウィンク");
		//float weight = m_pModels[0]->GetShapeWeight("Body", shapeIndex);
		weight += g_Engine->GetFrameTime();
		m_pModels[0]->SetShapeWeight("ウィンク", weight);
		//m_pModels[0]->SetShapeWeight("Body", shapeIndex, weight);
	}

	if (g_Engine->GetKeyState(DIK_G)) {
		animSpeed -= 0.005f;
		if (animSpeed < 0.1f)
			animSpeed = 0.1f;
		m_pModels[0]->m_animationSpeed = animSpeed;
		m_pBGMHandle->m_speed = animSpeed;
		m_pBGMHandle->UpdateProperty();
	}
	if (g_Engine->GetKeyState(DIK_H)) {
		animSpeed += 0.005f;
		if (animSpeed > 2.0f)
			animSpeed = 2.0f;
		m_pModels[0]->m_animationSpeed = animSpeed;
		m_pBGMHandle->m_speed = animSpeed;
		m_pBGMHandle->UpdateProperty();
	}
	if (g_Engine->GetKeyState(DIK_T)) {
		m_pModels[0]->m_position.y -= 0.01f;
	}
	if (g_Engine->GetKeyState(DIK_Y)) {
		m_pModels[0]->m_position.y += 0.01f;
	}

	//m_physicsManager.UpdatePhysics(g_Engine->GetFrameTime());
}

void Scene::Draw()
{
	g_Engine->ModelRender();
}

Scene::Scene()
	: m_pModel2(nullptr)
	, m_pBGMHandle(nullptr)
{
}

Scene::~Scene()
{
	
}

void Scene::UpdateCamera()
{
	Camera* pCamera = g_Engine->GetCamera();

	//カメラの前方ベクトルを計算
	XMVECTOR forward = XMVector3Normalize(pCamera->m_targetPos - pCamera->m_eyePos);

	//カメラの右方向ベクトルを計算
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(pCamera->m_upFoward, forward));

	const float cameraSpeed = 1.2f * g_Engine->GetFrameTime();
	const float rotationSpeed = 0.1f * g_Engine->GetFrameTime();

	if (g_Engine->GetKeyState(DIK_SPACE)) {
		pCamera->m_eyePos = XMVectorAdd(pCamera->m_eyePos, XMVectorScale(pCamera->m_upFoward, cameraSpeed));
		pCamera->m_targetPos = XMVectorAdd(pCamera->m_targetPos, XMVectorScale(pCamera->m_upFoward, cameraSpeed));
	}
	else if (g_Engine->GetKeyState(DIK_LSHIFT)) {
		pCamera->m_eyePos = XMVectorSubtract(pCamera->m_eyePos, XMVectorScale(pCamera->m_upFoward, cameraSpeed));
		pCamera->m_targetPos = XMVectorSubtract(pCamera->m_targetPos, XMVectorScale(pCamera->m_upFoward, cameraSpeed));
	}

	//Move Right and Left
	if (g_Engine->GetKeyState(DIK_A)) {
		pCamera->m_eyePos = XMVectorAdd(pCamera->m_eyePos, XMVectorScale(right, cameraSpeed));
		pCamera->m_targetPos = XMVectorAdd(pCamera->m_targetPos, XMVectorScale(right, cameraSpeed));
	}
	else if (g_Engine->GetKeyState(DIK_D)) {
		pCamera->m_eyePos = XMVectorSubtract(pCamera->m_eyePos, XMVectorScale(right, cameraSpeed));
		pCamera->m_targetPos = XMVectorSubtract(pCamera->m_targetPos, XMVectorScale(right, cameraSpeed));
	}

	//Move forward/Backward
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
