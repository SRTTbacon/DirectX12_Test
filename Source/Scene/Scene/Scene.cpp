#include "Scene.h"

Scene* g_Scene = nullptr;

using namespace DirectX;

const std::string modelFile1 = "Resource/Model/Milltina/Milltina.hcs";
//const std::string modelFile1 = "Resource/Model/FBX_Moe.hcs";
const std::string modelFile2 = "Resource/Model/Sphere.fbx";
const std::string modelFile3 = "Resource/Model/Plane.fbx";
const std::string modelFile4 = "Resource/Model/Gun.fbx";
const std::string modelFile5 = "Resource/Model/扇風機.fbx";

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
		pCharacter->AddAnimation(g_Engine->GetAnimation("Resource\\Test.hcs"));
		pCharacter->m_animationSpeed = animSpeed;

		//pCharacter->SetShapeWeight("Foot_heel", 1.0f);
		pCharacter->SetShapeWeight("Breasts_Flat", 1.0f);
		pCharacter->SetShapeWeight("Apron_set", 1.0f);
		//pCharacter->SetShapeWeight("eye_joy", 1.0f);

		pCharacter->SetTransparent(true);

		std::vector meshNames = { "Body", "Milltina_body", "Milltina_hair_base", "Milltina_hair_front", "Milltina_hair_front_side", "Milltina_hair_side", "Milltina_hair_twintail",
		"Milltina_cloth_dress", "Milltina_cloth_skirt", "Milltina_cloth_neck_ribbon", "Milltina_cloth_hat", "Milltina_cloth_hat", "Milltina_cloth_hair_ribbon", "Milltina_cloth_garterbelt",
		"Milltina_cloth_wrist_cuffs", "Milltina_cloth_apron"};
		//std::vector meshNames = { "Body", "Body all", "Hair"};
		std::vector<Character::HumanoidMesh>& meshes = pCharacter->GetHumanMeshes();
		for (int i = 0; i < static_cast<int>(meshes.size()); i++) {
			Character::HumanoidMesh& mesh = meshes[i];
			mesh.pMesh->bDraw = false;
			printf("MeshName = %s\n", mesh.pMesh->meshName.c_str());
			for (const std::string& meshName : meshNames) {
				if (mesh.pMesh->meshName == meshName) {
					if (mesh.pMesh && !pCharacter->GetTexture(i)->IsSimpleTex()) {
						mesh.pMesh->bDraw = true;
					}
				}
			}
		}

		//m_pDynamicBone = new DynamicBone(m_bullet.GetDynamicWorld(), &pCharacter->m_boneManager);
		//m_pDynamicBone->Initialize(m_pModels[0]->m_boneManager.GetBone("Hair Root"), 0.5f, 0.2f);
	}

	/*for (UINT i = 0; i < m_pModels[0]->m_boneManager.m_bones.size(); i++) {
		if (i > 200) {
			break;
		}
		Bone& bone = m_pModels[0]->m_boneManager.m_bones[i];
		Model* pSphere = g_Engine->AddModel(modelFile2);
		pSphere->m_scale = XMFLOAT3(0.01f, 0.01f, 0.01f);
		XMMATRIX m = m_pModels[0]->m_boneManager.m_finalBoneTransforms[bone.GetBoneName()];
		XMMATRIX rotX = XMMatrixRotationX(XMConvertToRadians(-90.0f));
		XMMATRIX rotY = XMMatrixRotationY(XMConvertToRadians(180.0f));
		XMMATRIX rot = rotX * rotY;
		m *= rot;
		pSphere->m_position.x = m.r[3].m128_f32[0];
		pSphere->m_position.y = m.r[3].m128_f32[1];
		pSphere->m_position.z = m.r[3].m128_f32[2];
		m_spheres.push_back(pSphere);
		
	}*/
	//地面
	m_pModel2 = g_Engine->AddModel(modelFile3);
	m_pModel2->m_scale = XMFLOAT3(50.0f, 0.05f, 50.0f);
	m_pModel2->m_position.y = -0.05f;

	m_pModel3 = g_Engine->AddModel(modelFile3);
	m_pModel3->m_scale = XMFLOAT3(0.5f, 0.5f, 0.5f);
	m_pModel3->m_position = XMFLOAT3(-1.0f, 0.5f, 1.0f);

	m_pModel4 = g_Engine->AddModel(modelFile4);
	m_pModel4->m_rotation.x = -90.0f;
	m_pModel4->m_position.y = 1.0f;
	m_pModel4->m_scale = XMFLOAT3(0.25f, 0.25f, 0.25f);

	for (int i = 0; i < 1; i++) {
		m_pModels[i]->m_rotation.x = -90.0f;
		m_pModels[i]->m_rotation.y = 180.0f;
		if (i > 0) {
			m_pModels[i]->m_position.x = (float)((i % 2 == 0 ? 1.0f : -1.0f) * i / 2) + 1.0f;
		}
	}

	m_pBGMHandle = g_Engine->GetSoundSystem()->LoadSound("Resource\\BGM\\VRSuya - Doodle Dance.wav", true);
	m_pBGMHandle->m_volume = 0.25f;
	m_pBGMHandle->m_speed = animSpeed;
	m_pBGMHandle->m_bLooping = true;
	m_pBGMHandle->UpdateProperty();

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
	if (g_Engine->GetMouseState(0x01)) {
		m_pModels[0]->m_rotation.y += 2.0f;
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

	if (g_Engine->GetKeyState(DIK_N)) {
		float weight = m_pModels[0]->GetShapeWeight("High heeled_ON");
		//float weight = m_pModels[0]->GetShapeWeight("Body", shapeIndex);
		weight -= g_Engine->GetFrameTime();
		m_pModels[0]->SetShapeWeight("High heeled_ON", weight);

		weight = m_pModels[0]->GetShapeWeight("High heeled_OFF");
		weight += g_Engine->GetFrameTime();
		m_pModels[0]->SetShapeWeight("High heeled_OFF", weight);
	}
	if (g_Engine->GetKeyState(DIK_M)) {
		float weight = m_pModels[0]->GetShapeWeight("High heeled_ON");
		weight += g_Engine->GetFrameTime();
		m_pModels[0]->SetShapeWeight("High heeled_ON", weight);

		weight = m_pModels[0]->GetShapeWeight("High heeled_OFF");
		weight -= g_Engine->GetFrameTime();
		m_pModels[0]->SetShapeWeight("High heeled_OFF", weight);
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

	if (g_Engine->GetKeyState(DIK_UP)) {
		m_pModel4->m_position.z -= 0.01f;
	}
	if (g_Engine->GetKeyState(DIK_DOWN)) {
		m_pModel4->m_position.z += 0.01f;
	}
	if (g_Engine->GetKeyState(DIK_LEFT)) {
		m_pModel4->m_position.x -= 0.01f;
	}
	if (g_Engine->GetKeyState(DIK_RIGHT)) {
		m_pModel4->m_position.x += 0.01f;
	}

	if (g_Engine->GetKeyState(DIK_1)) {
		g_Engine->GetCamera()->m_test -= 0.01f;
		if (g_Engine->GetCamera()->m_test < 0.0f) {
			g_Engine->GetCamera()->m_test = 0.0f;
		}
	}
	if (g_Engine->GetKeyState(DIK_2)) {
		g_Engine->GetCamera()->m_test += 0.01f;
		if (g_Engine->GetCamera()->m_test > 1.0f) {
			g_Engine->GetCamera()->m_test = 1.0f;
		}
	}

	if (g_Engine->GetKeyStateSync(DIK_C)) {
		ConvertFromFBX convert;
		//convert.ConvertFromCharacter(m_pModels[0], modelFile1, "Resource/Model/Milltina/Milltina.hcs");
		std::vector<Character::HumanoidMesh>& meshes = m_pModels[0]->GetHumanMeshes();
		for (Character::HumanoidMesh& mesh : meshes) {
			//printf("MeshName = %s\n", mesh.pMesh->meshName.c_str());
		}

		Character::HumanoidMesh* a = m_pModels[0]->GetHumanMesh("Body all");
		for (std::pair<std::string, UINT> keyValue : a->shapeMapping) {
			printf("%s\n", keyValue.first.c_str());
		}
	}

	m_pModels[0]->Update();
	m_bullet.Update(g_Engine->GetFrameTime());
	//m_pDynamicBone->Update();
}

void Scene::Draw()
{
	g_Engine->ModelRender();
}

Scene::Scene()
	: m_pModel2(nullptr)
	, m_pModel3(nullptr)
	, m_pBGMHandle(nullptr)
	, m_pDynamicBone(nullptr)
	, m_bullet(XMFLOAT3(0.0f, -9.81f, 0.0f))
{
}

Scene::~Scene()
{
	for (size_t i = 0; i < m_pModels.size(); i++) {
		//delete m_pModels[i];
	}
	//m_pModels.clear();
}

void Scene::UpdateCamera()
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
