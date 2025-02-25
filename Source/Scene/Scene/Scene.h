#pragma once
#include "..\\..\\System\\Engine\\Engine.h"
#include "..\\..\\System\\Engine\\Model\\Convert\\ConvertFromFBX.h"
#include <DirectXMath.h>
#include "..\\..\\System\\Engine\\Physics\\PhysXManager.h"

class Scene
{
public:
	bool Init(); //‰Šú‰»

	void Update(); //XVˆ—
	void Draw(); //•`‰æˆ—

	Scene();
	~Scene();

private:
	Character* m_pChar1;
	Character* m_pChar2;
	Model* m_pModel2;
	Model* m_pModel3;
	Model* m_pModel4;
	Model* m_pModel5;
	SoundHandle* m_pBGMHandle;

	PhysXManager m_physX;

	std::vector<Model*> m_spheres;

	void UpdateCamera();
};

extern Scene* g_Scene;