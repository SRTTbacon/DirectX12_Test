#pragma once
#include "..\\..\\System\\Engine\\Engine.h"
#include "..\\..\\System\\Engine\\Model\\Convert\\ConvertFromFBX.h"
#include "..\\..\\System\\Engine\\Model\\BulletPhysics\\DynamicBone.h"
#include <DirectXMath.h>

class Scene
{
public:
	bool Init(); //������

	void Update(); //�X�V����
	void Draw(); //�`�揈��

	Scene();
	~Scene();

private:
	std::vector<Character*> m_pModels;
	Model* m_pModel2;
	Model* m_pModel3;
	SoundHandle* m_pBGMHandle;

	BulletPhysics m_bullet;
	DynamicBone* m_pDynamicBone;

	std::vector<Model*> m_spheres;

	void UpdateCamera();
};

extern Scene* g_Scene;