#pragma once
#include "..\\..\\System\\Engine\\Engine.h"
#include <DirectXMath.h>

class Scene
{
public:
	bool Init(); // 初期化

	void Update(); // 更新処理
	void Draw(); // 描画処理

	Scene();

private:
	Character* m_pModel1;
	Model* m_pModel2;

	SoundHandle* pBGMHandle;

	std::vector<Model*> m_spheres;

	void UpdateCamera();
};

extern Scene* g_Scene;