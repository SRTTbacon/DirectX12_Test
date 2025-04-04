#pragma once
#include <DirectXMath.h>

#include "..\\Scene\\Scene.h"

#include "..\\..\\System\\Engine\\Engine.h"
#include "..\\..\\System\\Engine\\Physics\\PhysXManager.h"
#include "..\\..\\System\\Engine\\Physics\\CharacterPhysics\\HairPhysicsBone.h"

#include "..\\..\\Object\\Terrain\\Terrain.h"

class SceneGame : public Scene
{
public:
	SceneGame();
	~SceneGame();

	void Start(); //初期化

	void Update(); //更新処理
	void Draw(); //描画処理

private:
	struct BoneSphere
	{
		Bone* pBone;
		Model* pModel;
	};

	Character* m_pChar1;
	Character* m_pChar2;
	BassSoundHandle* m_pBGMHandle;
	UITextureRef uiTexture;
	UITextRef uiText;

	std::vector<BoneSphere> m_spheres;

	//PhysXManager m_physX;
	//HairPhysicsBone m_hairPhysics;
	Terrain m_terrain;

	//ボーンの位置に球体を設置
	void CreateSphere(Bone* pBone);

	//カメラ移動
	void UpdateCamera();
};
