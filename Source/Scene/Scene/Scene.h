#pragma once
#include "..\\..\\System\\Engine\Model\Model.h"
#include "..\\..\\System\\Engine\\Core\\ConstantBuffer\\ConstantBuffer.h"
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

	Model m_model1;
};

extern Scene* g_Scene;