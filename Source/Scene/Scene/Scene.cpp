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
const std::string modelFile2 = "Resource/Model/FBX_Moe2.fbx";

bool Scene::Init()
{
	printf("シーンの初期化に成功\n");
	return true;
}

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
		m_model1.m_scale.x -= 0.005f;
	}
	if (g_Engine->GetKeyState(DIK_X) && g_Engine->GetKeyState(DIK_L)) {
		m_model1.m_scale.x += 0.005f;
	}
	if (g_Engine->GetKeyState(DIK_R) && g_Engine->GetKeyState(DIK_J)) {
		m_model1.m_rotation.y -= 0.5f;
	}
	if (g_Engine->GetKeyState(DIK_R) && g_Engine->GetKeyState(DIK_L)) {
		m_model1.m_rotation.y += 0.5f;
	}
	if (g_Engine->GetKeyState(DIK_P) && g_Engine->GetKeyState(DIK_J)) {
		m_model1.m_position.x -= 0.005f;
	}
	if (g_Engine->GetKeyState(DIK_P) && g_Engine->GetKeyState(DIK_L)) {
		m_model1.m_position.x += 0.005f;
	}

	m_model1.Update();
}

void Scene::Draw()
{
	m_model1.Draw(g_Engine->CommandList());
}

Scene::Scene() : m_model1(Model(g_Engine->Device(), g_Engine->CommandList(), modelFile1, &m_camera))
{
}
