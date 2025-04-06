#pragma once
#include <DirectXMath.h>

#include "..\\Scene\\Scene.h"

#include "..\\..\\System\\Engine\\Engine.h"
#include "..\\..\\System\\Engine\\Physics\\PhysXManager.h"
#include "..\\..\\System\\Engine\\Physics\\CharacterPhysics\\HairPhysicsBone.h"

#include "..\\..\\Object\\Terrain\\Terrain.h"

#include "..\\..\\Object\\Game\\GameCharacter.h"

class SceneGame : public Scene
{
public:
	SceneGame();
	~SceneGame();

	void Start(); //‰Šú‰»

	void Update(); //XVˆ—
	void Draw(); //•`‰æˆ—

private:
	struct BoneSphere
	{
		Bone* pBone;
		Model* pModel;
	};

	static const std::string FONT_MESSAGE;

	UITextureRef m_uiBlack;
	UITextRef m_uiText;
	UITextRef m_uiHelpText;

	std::vector<BoneSphere> m_spheres;

	//PhysXManager m_physX;
	//HairPhysicsBone m_hairPhysics;
	Terrain m_terrain;
	GameCharacter m_character;

	float m_sceneTime;

	//ƒJƒƒ‰ˆÚ“®
	void UpdateCamera();

	void UpdateKeys();
};
