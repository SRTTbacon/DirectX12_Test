#include "Scene.h"

Scene* g_Scene;

using namespace DirectX;

const std::string modelFile1 = "Resource/Model/FBX_Moe.fbx";
const std::string modelFile2 = "Resource/Model/Sphere.fbx";
const std::string modelFile3 = "Resource/Model/Plane.fbx";

bool Scene::Init()
{
	g_Engine->GetCamera()->SetFov(65.0f);

	printf("シーンの初期化に成功\n");
	return true;
}

bool bAnim = true;
bool bVisible = true;
bool bBoneMode = false;

void Scene::Update()
{
	UpdateCamera();

	if (g_Engine->GetMouseStateSync(0x00)) {
		bAnim = !bAnim;
		if (bAnim) {
			m_pModel1->m_animationSpeed = 1.0f;
			pBGMHandle->PlaySound(false);
		}
		else {
			m_pModel1->m_animationSpeed = 0.0f;
			pBGMHandle->PauseSound();
		}
	}
	if (g_Engine->GetMouseStateSync(0x01)) {
		m_pModel1->m_rotation.x += 90.0f;
	}
	if (g_Engine->GetMouseStateSync(0x02)) {
		pBGMHandle->SetPosition(m_pModel1->m_nowAnimationTime);
	}

	if (g_Engine->GetKeyStateSync(DIK_J)) {
		m_pModel1->m_nowAnimationTime -= 5.0f;
		if (m_pModel1->m_nowAnimationTime < 0.0f) {
			m_pModel1->m_nowAnimationTime = 0.0f;
		}

		pBGMHandle->SetPosition(m_pModel1->m_nowAnimationTime);
	}
	else if (g_Engine->GetKeyStateSync(DIK_K)) {
		m_pModel1->m_nowAnimationTime += 5.0f;
		pBGMHandle->SetPosition(m_pModel1->m_nowAnimationTime);
	}

	if (g_Engine->GetKeyState(DIK_N)) {
		g_Engine->GetDirectionalLight()->AddRotationX(-1.0f);
	}
	if (g_Engine->GetKeyState(DIK_M)) {
		g_Engine->GetDirectionalLight()->AddRotationX(1.0f);
	}

	if (g_Engine->GetKeyState(DIK_G)) {
		m_pModel1->GetBone("Right knee")->m_rotation.x -= XMConvertToRadians(2.0f);
	}
	if (g_Engine->GetKeyState(DIK_H)) {
		m_pModel1->GetBone("Right knee")->m_rotation.x += XMConvertToRadians(2.0f);
	}
	if (g_Engine->GetKeyState(DIK_T)) {
		m_pModel1->m_position.y -= 0.01f;
	}
	if (g_Engine->GetKeyState(DIK_Y)) {
		m_pModel1->m_position.y += 0.01f;
	}

	if (bBoneMode) {
		for (std::string boneName : m_pModel1->GetBoneNames()) {
			XMFLOAT3 a = XMFLOAT3(0.0f, 0.0f, 0.0f);
			XMFLOAT4 b = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
			m_pModel1->UpdateBonePosition(boneName, a);
			m_pModel1->UpdateBoneRotation(boneName, b);
		}
		m_pModel1->m_rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	}
	else {
		//m_model1.m_rotation.x = -90.0f;
		//m_model1.m_rotation.y = 180.0f;
	}
}

void Scene::Draw()
{
	g_Engine->ModelRender();
}

Scene::Scene() 
{
	m_pModel1 = g_Engine->AddCharacter(modelFile1);
	m_pModel1->AddAnimation(g_Engine->GetAnimation("Resource\\Test.hcs"));
	m_pModel1->m_animationSpeed = 1.0f;

	XMFLOAT4 a = {0.0f, 0.0f, 0.0f, 0.0f};
	/*XMFLOAT3 a = {-10.0f, 13.5f, 0.58f};
	m_model1.UpdateBoneRotation("Left leg", a);
	a = { 6.9f, -7.4f, -1.9f };
	m_model1.UpdateBoneRotation("Left knee", a);
	a = { 6.7f, -9.4f, -1.0f };
	m_model1.UpdateBoneRotation("Right leg", a);
	a = { 22.0f, 7.3f, 7.0f };
	m_model1.UpdateBoneRotation("Right knee", a);
	a = { 15.0f, 0.0f, 0.0f };
	m_model1.UpdateBoneRotation("Right ankle", a);
	//a = { -76.0f, -9.8f, 0.0f };
	a = { -7.0f, -9.8f, 0.0f };
	m_model1.UpdateBoneRotation("Left arm", a);

	//m_model1.m_position.x = 2.0f;
	*/
	//m_model2.LoadModel(Primitive_Sphere);
	/*for (std::string name : m_pModel1->GetBoneNames()) {
		Model* pModel = g_Engine->AddModel(modelFile2);
		XMMATRIX mat = m_pModel1->finalBoneTransforms[name];
		pModel->m_position.x = mat.r[3].m128_f32[0];
		pModel->m_position.y = mat.r[3].m128_f32[1];
		pModel->m_position.z = mat.r[3].m128_f32[2];
		pModel->m_scale = XMFLOAT3(0.05f, 0.05f, 0.05f);
		pModel->m_bVisible = false;
		m_spheres.push_back(pModel);
	}*/

	m_pModel2 = g_Engine->AddModel(modelFile3);
	m_pModel2->m_scale = XMFLOAT3(50.0f, 0.05f, 50.0f);
	m_pModel2->m_position.y = -0.2f;
	//m_pModel2->m_bVisible = false;

	m_pModel1->m_rotation.x = -90.0f;
	m_pModel1->m_rotation.y = 180.0f;

	pBGMHandle = g_Engine->GetSoundSystem()->LoadSound("Resource\\BGM\\君色に染まる.mp3", true);
	pBGMHandle->volume = 0.25f;
	pBGMHandle->speed = 1.0f;
	pBGMHandle->bLooping = true;
	pBGMHandle->UpdateProperty();
}

void Scene::UpdateCamera()
{
	Camera* pCamera = g_Engine->GetCamera();

	// カメラの前方ベクトルを計算
	XMVECTOR forward = XMVector3Normalize(pCamera->m_targetPos - pCamera->m_eyePos);

	// カメラの右方向ベクトルを計算
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(pCamera->m_upFoward, forward));

	const float cameraSpeed = 0.01f;
	const float rotationSpeed = 0.01f;

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

	//Rotate Y axis
	if (g_Engine->GetKeyState(DIK_RIGHT)) {
		pCamera->m_yaw -= rotationSpeed;
	}
	if (g_Engine->GetKeyState(DIK_LEFT)) {
		pCamera->m_yaw += rotationSpeed;
	}

	//Rotate X axis
	if (g_Engine->GetKeyState(DIK_DOWN)) {
		pCamera->m_pitch -= rotationSpeed;
	}
	if (g_Engine->GetKeyState(DIK_UP)) {
		pCamera->m_pitch += rotationSpeed;
	}
}
