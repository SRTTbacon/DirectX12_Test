#pragma once
#include "..\\..\\System\\Engine\Model\Character.h"
#include <DirectXMath.h>

class Scene
{
public:
	bool Init(); // 初期化

	void Update(); // 更新処理
	void Draw(); // 描画処理

	Scene();

private:
	Camera m_camera;

	Character m_model1;
	Model m_model2;

	SoundHandle* pBGMHandle;

	std::vector<Model> m_spheres;
};

extern Scene* g_Scene;