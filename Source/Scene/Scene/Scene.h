#pragma once
#include "..\\..\\System\\Engine\Model\Character.h"
#include <DirectXMath.h>
#include "..\\..\\System\\Engine\\Model\\Animation\\Animation.h"

class Scene
{
public:
	bool Init(); // ‰Šú‰»

	void Update(); // XVˆ—
	void Draw(); // •`‰æˆ—

	Scene();

private:
	Camera m_camera;

	Animation m_anim;
	Character m_model1;
	std::vector<Model> m_spheres;
};

extern Scene* g_Scene;