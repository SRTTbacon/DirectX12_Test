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
float x = 0.0f;
float y = 0.0f;
float z = 0.0f;
float w = 0.0f;
float x2 = 0.0f;
float y2 = 0.0f;
float z2 = 0.0f;
float w2 = 0.0f;

void Scene::Update()
{
	/*for (size_t i = 0; i < meshes.size(); i++)
	{
		auto size = sizeof(Vertex) * meshes[i].Vertices.size();
		auto stride = sizeof(Vertex);
		auto vertices = meshes[i].Vertices.data();
		for (size_t j = 0; j < meshes[i].Vertices.size(); j++)
			vertices[j].Position.x += 0.001f;

		vertexBuffers[i]->Update(size, stride, vertices);
	}*/

	//auto currentIndex = g_Engine->CurrentBackBufferIndex(); // 現在のフレーム番号を取得
	//auto currentTransform = m_camera.m_constantBuffer[currentIndex]->GetPtr<Transform>(); // 現在のフレーム番号に対応する定数バッファを取得

	UpdateCamera();

	if (g_Engine->GetKeyState(DIK_X) && g_Engine->GetKeyState(DIK_J)) {
		x -= 0.005f;
	}
	if (g_Engine->GetKeyState(DIK_X) && g_Engine->GetKeyState(DIK_L)) {
		x += 0.005f;
	}
	if (g_Engine->GetKeyState(DIK_C) && g_Engine->GetKeyState(DIK_J)) {
		y -= 0.005f;
	}
	if (g_Engine->GetKeyState(DIK_C) && g_Engine->GetKeyState(DIK_L)) {
		y += 0.005f;
	}
	if (g_Engine->GetKeyState(DIK_V) && g_Engine->GetKeyState(DIK_J)) {
		z -= 0.005f;
	}
	if (g_Engine->GetKeyState(DIK_V) && g_Engine->GetKeyState(DIK_L)) {
		z += 0.005f;
	}
	if (g_Engine->GetKeyState(DIK_B) && g_Engine->GetKeyState(DIK_J)) {
		w -= 0.005f;
	}
	if (g_Engine->GetKeyState(DIK_B) && g_Engine->GetKeyState(DIK_L)) {
		w += 0.005f;
	}

	if (g_Engine->GetKeyState(DIK_Y) && g_Engine->GetKeyState(DIK_J)) {
		x2 -= 0.005f;
	}
	if (g_Engine->GetKeyState(DIK_Y) && g_Engine->GetKeyState(DIK_L)) {
		x2 += 0.005f;
	}
	if (g_Engine->GetKeyState(DIK_U) && g_Engine->GetKeyState(DIK_J)) {
		y2 -= 0.005f;
	}
	if (g_Engine->GetKeyState(DIK_U) && g_Engine->GetKeyState(DIK_L)) {
		y2 += 0.005f;
	}
	if (g_Engine->GetKeyState(DIK_I) && g_Engine->GetKeyState(DIK_J)) {
		z2 -= 0.005f;
	}
	if (g_Engine->GetKeyState(DIK_I) && g_Engine->GetKeyState(DIK_L)) {
		z2 += 0.005f;
	}
	if (g_Engine->GetKeyState(DIK_O) && g_Engine->GetKeyState(DIK_J)) {
		w2 -= 0.005f;
	}
	if (g_Engine->GetKeyState(DIK_O) && g_Engine->GetKeyState(DIK_L)) {
		w2 += 0.005f;
	}
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
		m_pModel1->GetBone("Hips")->m_position.x -= 0.05f;
	}
	if (g_Engine->GetKeyState(DIK_H)) {
		m_pModel1->GetBone("Hips")->m_position.x += 0.05f;
	}
	if (g_Engine->GetKeyState(DIK_T)) {
		m_pModel1->m_position.y -= 0.01f;
	}
	if (g_Engine->GetKeyState(DIK_Y)) {
		m_pModel1->m_position.y += 0.01f;
	}

	//printf("x=%f, y=%f, z=%f, w=%f\n", x, y, z, w);
	//printf("x2=%f, y2=%f, z2=%f, w2=%f\n", x2, y2, z2, w2);

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

	XMFLOAT4 a = { x, y, z, w };
	//m_model1.UpdateBoneRotation("Hips", a);
	//m_model1.UpdateBoneRotation("Right arm", a);
	a = { x2, y2, z2, w2 };
	//m_model1.UpdateBoneRotation("Right elbow", a);

	//m_model1.Test();

	//printf("x=%f, y=%f, z=%f\n", m_pModel1->GetBone("Hips")->m_position.x, m_pModel1->GetBone("Hips")->m_position.y, m_pModel1->GetBone("Hips")->m_position.z);
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

	x = 0.13954f;
	y = -0.00043f;
	z = -0.00517f;
	w = 0.99020f;

	x2 = -0.00701f;
	y2 = 0.00789f;
	z2 = 0.01992f;
	w2 = 0.99975f;

	/*a.x = 0.11753f;
	a.y = 0.00093f;
	a.z = -0.16284f;
	a.w = -0.97963f;
	m_model1.UpdateBoneRotation("Hips", a);
	a.x = -0.11246f;
	a.y = 0.03627f;
	a.z = -0.00826f;
	a.w = 0.99296f;
	m_model1.UpdateBoneRotation("Spine", a);
	a.x = 0.11456f;
	a.y = 0.01250f;
	a.z = -0.05481f;
	a.w = 0.99182f;
	m_model1.UpdateBoneRotation("Chest", a);
	a.x = 0.48446f;
	a.y = -0.52316f;
	a.z = -0.44220f;
	a.w = -0.54412f;
	m_model1.UpdateBoneRotation("Left shoulder", a);
	a.x = 0.10031f;
	a.y = -0.11502f;
	a.z = -0.23487f;
	a.w = 0.95997f;
	m_model1.UpdateBoneRotation("Left arm", a);
	a.x = 0.14118f;
	a.y = -0.03243f;
	a.z = 0.91126f;
	a.w = 0.38552f;
	m_model1.UpdateBoneRotation("Left elbow", a);
	a.x = 0.22555f;
	a.y = -0.33024f;
	a.z = 0.03830f;
	a.w = 0.91575f;
	//m_model1.UpdateBoneRotation("Left wrist", a);*/

	m_pModel1->m_rotation.x = -90.0f;
	m_pModel1->m_rotation.y = 180.0f;

	pBGMHandle = g_Engine->GetSoundSystem()->LoadSound("Resource\\BGM\\VRSuya - Doodle Dance.mp3", true);
	pBGMHandle->volume = 0.1f;
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
