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

const wchar_t* modelFile1 = L"Resource/Model/FBX_Moe.fbx";
const wchar_t* modelFile2 = L"Resource/Model/FBX_Moe2.fbx";

bool Scene::Init()
{
	m_camera.SetFov(45.0f);

	m_model1.Initialize(modelFile1, &m_camera, true);
	m_model2.Initialize(modelFile2, &m_camera, true);

	m_model1.SetPosition(XMFLOAT3(-10.0f, 0.0f, 0.0f));
	m_model2.SetPosition(XMFLOAT3(-20.0f, 0.0f, 0.0f));

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

	m_model1.Update();
	m_model2.Update();
}

void Scene::Draw()
{
	m_model1.Draw();
	m_model2.Draw();
}

Scene::Scene()
{
}
