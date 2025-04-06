#pragma once
#include "..\\..\\GameBase.h"

class Terrain
{
public:
	Terrain();
	~Terrain();

	void Initialize();

	void Update();

private:
	static const std::string MODEL_BOX;
	static const std::string MODEL_MOUNTAIN_01;
	static const std::string MODEL_CLIFF_01;
	static const std::string MODEL_WING_01;
	static const std::string MODEL_GRASS_01;
	static const std::string TEXTURE_MOUNTAIN_01;
	static const std::string TEXTURE_ROCK_01;
	static const std::string TEXTURE_GRASS_01;
	static const std::string TEXTURE_GRASS_ALPHA_01;
	static const std::string TEXTURE_GRASS_NOISE_01;
	static const char* BLACKOUT_HCS_PATH;
	static const char* BLACKOUT_PATH;
	static const float BLACKOUT_FRAME_TIME;
	static const float BLACKOUT_WAIT_TIME;

	Model* m_pGround;
	Model* m_pMountain_01;
	Model* m_pCliff_01;
	Model* m_pCliff_02;
	Model* m_pWing_01;
	Model* m_pBlackout;
	std::vector<Model*> m_pGrasses;
	std::vector<Texture2D*> m_blackouts;

	Effect* m_pEffectPlanet;
	Effect* m_pEffectDust;
	EffectHandle* m_pHandlePlanet;

	int m_nowBlackoutFrame;
	int m_nowBlackoutCount;

	float m_blackoutTime;
};