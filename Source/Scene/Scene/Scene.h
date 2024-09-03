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
	Model m_model1;
	Model m_model2;
	Camera m_camera;
};

extern Scene* g_Scene;