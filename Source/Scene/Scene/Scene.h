#pragma once
#include "..\\..\\System\\Engine\\Engine.h"
#include "..\\..\\System\\Engine\\Model\\Convert\\ConvertFromFBX.h"
#include <DirectXMath.h>

class Scene
{
public:
	bool Init(); //‰Šú‰»

	void Update(); //XVˆ—
	void Draw(); //•`‰æˆ—

	Scene();
	~Scene();

private:
	std::vector<Character*> m_pModels;
	Model* m_pModel2;
	SoundHandle* m_pBGMHandle;

	std::vector<Model*> m_spheres;

	void UpdateCamera();
};

extern Scene* g_Scene;