#pragma once
#include "..\\..\\System\\Engine\Model\Character.h"
#include <DirectXMath.h>
#include "..\\..\\System\\Engine\\Model\\Animation\\Animation.h"

class Scene
{
public:
	bool Init(); // ������

	void Update(); // �X�V����
	void Draw(); // �`�揈��

	Scene();

private:
	Camera m_camera;

	Animation m_anim;
	Character m_model1;
	std::vector<Model> m_spheres;
};

extern Scene* g_Scene;