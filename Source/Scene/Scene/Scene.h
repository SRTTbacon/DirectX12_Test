#pragma once
#include "..\\..\\System\\Engine\Model\Character.h"
#include <DirectXMath.h>

class Scene
{
public:
	bool Init(); // ������

	void Update(); // �X�V����
	void Draw(); // �`�揈��

	Scene();

private:
	Camera m_camera;

	Character m_model1;
	std::vector<Model> m_spheres;
};

extern Scene* g_Scene;