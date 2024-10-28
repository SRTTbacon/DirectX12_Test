#pragma once
#include "..\\..\\System\\Engine\\Engine.h"
#include <DirectXMath.h>

class Scene
{
public:
	bool Init(); // ‰Šú‰»

	void Update(); // XVˆ—
	void Draw(); // •`‰æˆ—

	Scene();

private:
	Character* m_pModel1;
	Model* m_pModel2;

	SoundHandle* pBGMHandle;

	std::vector<Model> m_spheres;

	void UpdateCamera();
};

extern Scene* g_Scene;