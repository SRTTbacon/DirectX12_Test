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

bool Scene::Init()
{
	printf("シーンの初期化に成功\n");
	m_camera.SetFov(69.0f);
	return true;
}

float x = 0.0f;
float y = 0.0f;
float z = 0.0f;

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
		x -= 0.5f;
	}
	if (g_Engine->GetKeyState(DIK_X) && g_Engine->GetKeyState(DIK_L)) {
		x += 0.5f;
	}
	if (g_Engine->GetKeyState(DIK_R) && g_Engine->GetKeyState(DIK_J)) {
		y -= 0.5f;
	}
	if (g_Engine->GetKeyState(DIK_R) && g_Engine->GetKeyState(DIK_L)) {
		y += 0.5f;
	}
	if (g_Engine->GetKeyState(DIK_P) && g_Engine->GetKeyState(DIK_J)) {
		z -= 0.5f;
	}
	if (g_Engine->GetKeyState(DIK_P) && g_Engine->GetKeyState(DIK_L)) {
		z += 0.5f;
	}
	if (g_Engine->GetMouseStateSync(0x00)) {
		m_model1.m_rotation.x += -90.0f;
	}
	XMFLOAT3 a = { x, y, z };
	m_model1.UpdateBoneRotation("Left arm", a);

	printf("x=%f, y=%f, z=%f\n", x, y, z);

	for (BoneAnimation& bone : m_anim.m_boneAnim) {
		/*if (bone.boneName == "Left leg") {
			m_model1.UpdateBoneRotation(bone.boneName, bone.rotation);
		}
		if (bone.boneName == "Left knee") {
			m_model1.UpdateBoneRotation(bone.boneName, bone.rotation);
			printf("rotationX = %f, rotationY = %f, rotationZ = %f\n", bone.rotation.x, bone.rotation.y, bone.rotation.z);
		}
		if (bone.boneName == "Right leg") {
			m_model1.UpdateBoneRotation(bone.boneName, bone.rotation);
		}
		if (bone.boneName == "Right knee") {
			m_model1.UpdateBoneRotation(bone.boneName, bone.rotation);
		}*/
		//m_model1.UpdateBonePosition(bone.boneName, bone.position);
		//m_model1.UpdateBoneRotation(bone.boneName, bone.rotation);
	}

	m_model1.Update();
	for (Model& model : m_spheres) {
		model.Update();
	}
}

void Scene::Draw()
{
	m_model1.Draw();
	for (Model& model : m_spheres) {
		model.Draw();
	}
}

Scene::Scene() 
	: m_model1(Character(modelFile1, &m_camera))
	, m_anim("Resource\\Test.hcs")
{
	XMFLOAT3 a = { -10.0f, 13.5f, 0.58f };
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

	for (BoneAnimation& bone : m_anim.m_boneAnim) {
		Bone* boneInfo = m_model1.GetBone(bone.boneName);
		boneInfo->m_boneOffset = XMMatrixTranslation(bone.initPosition.x, -bone.initPosition.z, bone.initPosition.y);
	}

	//m_model1.m_position.x = 2.0f;

	//m_model2.LoadModel(Primitive_Sphere);
	for (std::string name : m_model1.GetBoneNames()) {
		if (name == "Chest" || name == "Left arm" || name == "Left elbow") {
			Model model(&m_camera);
			model.LoadModel(modelFile2);
			model.m_position = m_model1.GetBoneOffset(name);
			model.m_scale = XMFLOAT3(0.05f, 0.05f, 0.05f);
			m_spheres.push_back(model);
		}
	}
}
