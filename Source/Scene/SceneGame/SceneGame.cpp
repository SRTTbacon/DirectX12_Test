#include "SceneGame.h"

#include "..\\..\\System\\Engine\\Model\\Convert\\ConvertFromFBX.h"

using namespace DirectX;

const std::string modelFile1 = "Resource/Model/FBX_Moe.hcs";
const std::string modelSphere = "Resource/Model/Sphere.fbx";

bool bAnim = true;
bool bVisible = true;
float animSpeed = 0.95f;

SceneGame::SceneGame()
	: m_pChar1(nullptr)
	, m_pChar2(nullptr)
	, m_pBGMHandle(nullptr)
{
}

SceneGame::~SceneGame()
{
}

void SceneGame::Start()
{
	g_Engine->GetCamera()->SetFov(85.0f);

	g_Engine->SetWindowMode(WindowMode::WindowMaximum, nullptr);

	//キャラクター
	m_pChar1 = g_Engine->AddCharacter(modelFile1);
	m_pChar1->AddAnimation(g_Engine->GetAnimation("Resource\\Roki1.hcs"));
	m_pChar1->m_animationSpeed = animSpeed;
	m_pChar1->SetRotation(-90.0f, 180.0f, 0.0f);
	m_pChar1->m_position.y = -1.08f;

	//CreateSphere(m_pChar1->GetBoneManager()->GetBone("Hair Root"));

	m_pChar2 = g_Engine->AddCharacter(modelFile1);
	m_pChar2->AddAnimation(g_Engine->GetAnimation("Resource\\Roki2.hcs"));
	m_pChar2->m_animationSpeed = animSpeed;
	m_pChar2->SetRotation(-90.0f, 180.0f, 0.0f);
	m_pChar2->m_position.y = -1.08f;

	/*m_hairPhysics.Initialize(m_physX.GetPhysics(), m_physX.GetScene());
	m_hairPhysics.SetBone(m_pChar1->GetBoneManager(), m_pChar1->GetBone("Hair Root"));*/

	/*std::vector meshNames = {"Body", "Milltina_body", "Milltina_hair_base", "Milltina_hair_front", "Milltina_hair_front_side", "Milltina_hair_side", "Milltina_hair_twintail",
	"Milltina_cloth_dress", "Milltina_cloth_skirt", "Milltina_cloth_neck_ribbon", "Milltina_cloth_hat", "Milltina_cloth_hat", "Milltina_cloth_hair_ribbon", "Milltina_cloth_garterbelt",
	"Milltina_cloth_wrist_cuffs", "Milltina_cloth_apron"};
	std::vector<Character::HumanoidMesh>& meshes = pCharacter->GetHumanMeshes();
	for (int i = 0; i < static_cast<int>(meshes.size()); i++) {
		Character::HumanoidMesh& mesh = meshes[i];
		//mesh.pMesh->bDraw = false;
		printf("MeshName = %s\n", mesh.pMesh->meshName.c_str());
		for (const std::string& meshName : meshNames) {
			if (mesh.pMesh->meshName == meshName) {
				if (mesh.pMesh && !pCharacter->GetTexture(i)->IsSimpleTex()) {
					mesh.pMesh->bDraw = true;
				}
			}
		}
	}*/

	m_pBGMHandle = g_Engine->GetBassSoundSystem()->LoadSound("Resource\\BGM\\Roki.mp3", false);
	m_pBGMHandle->m_volume = 0.1f;
	m_pBGMHandle->m_speed = animSpeed;
	m_pBGMHandle->m_bLooping = true;
	m_pBGMHandle->UpdateProperty();

	m_terrain.Initialize();
	uiTexture = g_Engine->GetUIManager()->AddUITexture("Resource\\Model\\Hair.png");
	uiTexture->m_size = XMFLOAT2(700.0f, 200.0f);
	uiTexture->m_position = XMFLOAT2(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f);
	uiTexture->m_color.a = 0.4f;

	uiText = g_Engine->GetUIManager()->AddUIText("Resource\\Fonts\\Test.ttc", 80);
	uiText->m_text = "abああ漢字こんにちんちん！！\nああああ";
	uiText->m_size = XMFLOAT2(1.0f, 1.0f);
	uiText->m_position = XMFLOAT2(100.0f, 100.0f);
	uiText->m_color = Color::CYAN;
	uiText->SetTextDistance(XMFLOAT2(10.0f, 10.0f));

	printf("シーンの初期化に成功\n");
}

void SceneGame::Update()
{
	UpdateCamera();

	if (g_Engine->GetMouseStateSync(0x00)) {
		bAnim = !bAnim;
		if (bAnim) {
			m_pChar1->m_animationSpeed = animSpeed;
			m_pChar2->m_animationSpeed = animSpeed;
			m_pBGMHandle->PlaySound(false);
		}
		else {
			m_pChar1->m_animationSpeed = 0.0f;
			m_pChar2->m_animationSpeed = 0.0f;
			m_pBGMHandle->PauseSound();
		}
	}
	if (g_Engine->GetMouseState(0x01)) {
		m_pChar1->m_rotation.y += 2.0f;
		m_pChar2->m_rotation.y += 2.0f;
	}
	if (g_Engine->GetMouseStateSync(0x02)) {
		m_pBGMHandle->SetPosition(m_pChar1->m_nowAnimationTime);
	}

	if (g_Engine->GetKeyStateSync(DIK_J)) {
		m_pChar1->m_nowAnimationTime -= 5.0f;
		m_pChar2->m_nowAnimationTime -= 5.0f;
		if (m_pChar1->m_nowAnimationTime < 0.0f) {
			m_pChar1->m_nowAnimationTime = 0.0f;
			m_pChar2->m_nowAnimationTime = 0.0f;
		}

		m_pBGMHandle->SetPosition(m_pChar1->m_nowAnimationTime);
	}
	else if (g_Engine->GetKeyStateSync(DIK_K)) {
		m_pChar1->m_nowAnimationTime += 5.0f;
		m_pChar2->m_nowAnimationTime += 5.0f;

		m_pBGMHandle->SetPosition(m_pChar1->m_nowAnimationTime);
	}

	//N または Mキーで環境光の角度を変更
	if (g_Engine->GetKeyState(DIK_N)) {
		g_Engine->GetDirectionalLight()->AddRotationX(-0.5f);
	}
	if (g_Engine->GetKeyState(DIK_M)) {
		g_Engine->GetDirectionalLight()->AddRotationX(0.5f);
	}

	if (g_Engine->GetKeyState(DIK_G)) {
		animSpeed -= 0.005f;
		if (animSpeed < 0.1f)
			animSpeed = 0.1f;
		m_pChar1->m_animationSpeed = animSpeed;
		m_pChar2->m_animationSpeed = animSpeed;
		m_pBGMHandle->m_speed = animSpeed;
		m_pBGMHandle->UpdateProperty();
	}
	if (g_Engine->GetKeyState(DIK_H)) {
		animSpeed += 0.005f;
		if (animSpeed > 2.0f)
			animSpeed = 2.0f;
		m_pChar1->m_animationSpeed = animSpeed;
		m_pChar2->m_animationSpeed = animSpeed;
		m_pBGMHandle->m_speed = animSpeed;
		m_pBGMHandle->UpdateProperty();
	}
	if (g_Engine->GetKeyState(DIK_T)) {
		m_pChar1->GetMesh(0)->m_position.y -= 0.01f;
		//m_pChar2->m_position.y -= 0.01f;
	}
	if (g_Engine->GetKeyState(DIK_Y)) {
		m_pChar1->GetMesh(0)->m_position.y += 0.01f;
		//m_pChar2->m_position.y += 0.01f;
	}

	uiText->m_position.x = static_cast<float>(g_Engine->GetMousePosition().x);
	uiText->m_position.y = static_cast<float>(g_Engine->GetMousePosition().y);

	if (g_Engine->GetKeyStateSync(DIK_C)) {
		ConvertFromFBX convert;
		//convert.ConvertFromCharacter(m_pChar1, modelFile1, "Resource/Model/FBX_Moe.hcs");
		//std::vector<Character::HumanoidMesh>& meshes = m_pModels[0]->GetHumanMeshes();
		//for (Character::HumanoidMesh& mesh : meshes) {
			//printf("MeshName = %s\n", mesh.pMesh->meshName.c_str());
		//}

		//Character::HumanoidMesh* a = m_pModels[0]->GetHumanMesh("Body all");
		//for (std::pair<std::string, UINT> keyValue : a->shapeMapping) {
			//printf("%s\n", keyValue.first.c_str());
		//}
	}

	m_pChar1->Update();
	m_pChar2->Update();

	for (BoneSphere& bs : m_spheres) {
		XMMATRIX test = bs.pBone->GetGlobalTransform();
		bs.pModel->m_position.x = test.r[3].m128_f32[0];
		bs.pModel->m_position.y = test.r[3].m128_f32[1];
		bs.pModel->m_position.z = test.r[3].m128_f32[2];
	}

	m_terrain.Update();
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

	uiTexture->Draw();
	uiText->Draw();
}

void SceneGame::CreateSphere(Bone* pBone)
{
	Model* pSphere = g_Engine->AddModel(modelSphere);
	pSphere->m_scale = XMFLOAT3(0.025f, 0.025f, 0.025f);

	BoneSphere bs = BoneSphere(pBone, pSphere);

	//m_spheres.push_back(bs);

	for (UINT i = 0; i < pBone->GetChildBoneCount(); i++) {
		CreateSphere(pBone->GetChildBone(i));
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
