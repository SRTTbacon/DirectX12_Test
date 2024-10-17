#include "Scene.h"

// 拡張子を置き換える処理
#include <filesystem>
namespace fs = std::filesystem;
static std::wstring ReplaceExtension(const std::wstring& origin, const char* ext)
{
	fs::path p = origin.c_str();
	return p.replace_extension(ext).c_str();
}

Scene* g_Scene;

using namespace DirectX;

const std::string modelFile1 = "Resource/Model/FBX_Moe.fbx";
const std::string modelFile2 = "Resource/Model/Sphere.fbx";
const std::string modelFile3 = "Resource/Model/IS7.fbx";

bool Scene::Init()
{
	printf("シーンの初期化に成功\n");
	m_camera.SetFov(69.0f);
	return true;
}

bool bAnim = true;
bool bVisible = false;
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

	m_camera.Update();

	if (g_Engine->GetKeyState(DIK_X) && g_Engine->GetKeyState(DIK_J)) {
		x -= 0.025f;
	}
	if (g_Engine->GetKeyState(DIK_X) && g_Engine->GetKeyState(DIK_L)) {
		x += 0.025f;
	}
	if (g_Engine->GetKeyState(DIK_R) && g_Engine->GetKeyState(DIK_J)) {
		y -= 0.025f;
	}
	if (g_Engine->GetKeyState(DIK_R) && g_Engine->GetKeyState(DIK_L)) {
		y += 0.025f;
	}
	if (g_Engine->GetKeyState(DIK_P) && g_Engine->GetKeyState(DIK_J)) {
		z -= 0.005f;
	}
	if (g_Engine->GetKeyState(DIK_P) && g_Engine->GetKeyState(DIK_L)) {
		z += 0.005f;
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
			m_model1.m_animationSpeed = 0.4f;
		}
		else {
			m_model1.m_animationSpeed = 0.0f;
		}
	}
	if (g_Engine->GetMouseStateSync(0x01)) {
		m_model1.xFlip = rand() % 2 == 0 ? -1.0f : 1.0f;
		m_model1.zFlip = rand() % 2 == 0 ? -1.0f : 1.0f;
		m_model1.yFlip = rand() % 2 == 0 ? -1.0f : 1.0f;
		m_model1.wFlip = rand() % 2 == 0 ? -1.0f : 1.0f;

		printf("x=%f, z=%f, y=%f, z=%f\n", m_model1.xFlip, m_model1.zFlip, m_model1.yFlip, m_model1.wFlip);
	}
	if (g_Engine->GetMouseStateSync(0x02)) {
		bBoneMode = !bBoneMode;
	}

	XMFLOAT4 a = { x, y, z, w };
	//m_model1.UpdateBoneRotation("Hips", a);
	a = { x2, y2, z2, z };
	//m_model1.UpdateBoneRotation("Left arm", a);

	m_model1.SetShapeWeight("目_笑い", x);
	m_model1.SetShapeWeight("あ", y);
	m_model1.SetShapeWeight("UP ear.UP ear", z);
	//m_model1.SetShapeWeight(aaa - 1, x);
	//m_model1.SetShapeWeight(aaa - 2, x);
	x = m_model1.GetShapeWeight("目_笑い");
	y = m_model1.GetShapeWeight("あ");
	z = m_model1.GetShapeWeight("UP ear.UP ear");

	//x = m_model1.GetShapeWeight("vrc.v_ch");

	//printf("Blinking = %f\n", x);

	//printf("x=%f, y=%f, z=%f\n", x, y, z);
	//printf("x=%f, y=%f, z=%f, w=%f\n", x2, y2, z2, z);

	if (bBoneMode) {
		for (std::string boneName : m_model1.GetBoneNames()) {
			XMFLOAT3 a = XMFLOAT3(0.0f, 0.0f, 0.0f);
			XMFLOAT4 b = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
			m_model1.UpdateBonePosition(boneName, a);
			m_model1.UpdateBoneRotation(boneName, b);
		}
		m_model1.m_rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	}
	else {
		m_model1.m_rotation.x = -90.0f;
		m_model1.m_rotation.y = 180.0f;
	}

	m_model1.Update();
	m_model2.Update();
	for (Model& model : m_spheres) {
		model.Update();
	}
}

void Scene::Draw()
{
	m_model1.Draw();
	if (bVisible)
		m_model2.Draw();
	for (Model& model : m_spheres) {
		model.Draw();
	}
}

Scene::Scene() 
	: m_model1(Character(modelFile1, &m_camera))
	, m_model2(Model(&m_camera))
{
	m_model1.LoadAnimation("Resource\\Test.hcs");

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

	//m_model2.LoadModel(Primitive_Sphere);
	for (std::string name : m_model1.GetBoneNames()) {
		if (name[0] == 'L' || name[0] == 'R') {
			Model model(&m_camera);
			model.LoadModel(modelFile2);
			XMMATRIX mat = m_model1.finalBoneTransforms[name];
			model.m_position.x = mat.r[3].m128_f32[0];
			model.m_position.y = mat.r[3].m128_f32[1];
			model.m_position.z = mat.r[3].m128_f32[2];
			printf("Name = %s, x=%f, y=%f, z=%f\n", name.c_str(), model.m_position.x, model.m_position.y, model.m_position.z);
			model.m_scale = XMFLOAT3(0.05f, 0.05f, 0.05f);
			m_spheres.push_back(model);
		}
	}*/

	m_model2.LoadModel(modelFile3);
	m_model2.m_scale = XMFLOAT3(0.01f, 0.01f, 0.01f);

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

	m_model1.m_rotation.x = -90.0f;
	m_model1.m_rotation.y = 180.0f;
}
