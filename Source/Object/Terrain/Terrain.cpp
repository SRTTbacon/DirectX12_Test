#include "Terrain.h"
#include "..\\..\\System\\Engine\\Model\\Convert\\ConvertFromFBX.h"
#include "..\\..\\Object\\FileSystem\\HCSFileSystem.h"

const std::string Terrain::MODEL_BOX = "Resource\\Model\\Plane.hcs";
const std::string Terrain::MODEL_MOUNTAIN_01 = "Resource\\Model\\Terrains\\Mountains\\SM_Terrain_02.hcs";
const std::string Terrain::MODEL_CLIFF_01 = "Resource\\Model\\Terrains\\Cliffs\\Cliff04.hcs";
const std::string Terrain::MODEL_WING_01 = "Resource\\Model\\扇風機.hcs";
const std::string Terrain::MODEL_GRASS_01 = "Resource\\Model\\Terrains\\Grass\\Grass01.hcs";
const std::string Terrain::TEXTURE_MOUNTAIN_01 = "Resource\\Model\\Terrains\\Textures\\Mountain_02_C.png";
const std::string Terrain::TEXTURE_ROCK_01 = "Resource\\Model\\Terrains\\Textures\\Cliff04_a.png";
const std::string Terrain::TEXTURE_GRASS_01 = "Resource\\Model\\Terrains\\Textures\\Grass01_a.png";
const std::string Terrain::TEXTURE_GRASS_ALPHA_01 = "Resource\\Model\\Terrains\\Grass\\Grass01.png";
const std::string Terrain::TEXTURE_GRASS_NOISE_01 = "Resource\\Model\\Terrains\\Grass\\NoiseGrass.png";

const char* Terrain::BLACKOUT_HCS_PATH = "Resource\\Effects\\Blackout.hcs";
const char* Terrain::BLACKOUT_PATH = "Resource\\Effects\\Blackout\\";
const float Terrain::BLACKOUT_FRAME_TIME = 0.0275f;
const float Terrain::BLACKOUT_WAIT_TIME = 1.5f;

Terrain::Terrain()
	: m_pGround(nullptr)
	, m_pMountain_01(nullptr)
	, m_pCliff_01(nullptr)
	, m_pCliff_02(nullptr)
	, m_pBlackout(nullptr)
	, m_pWing_01(nullptr)
	, m_pEffectPlanet(nullptr)
	, m_pEffectDust(nullptr)
	, m_pHandlePlanet(nullptr)
	, m_nowBlackoutFrame(-1)
	, m_nowBlackoutCount(0)
	, m_blackoutTime(0.0f)
{
}

Terrain::~Terrain()
{
	for (Texture2D* pTexture : m_blackouts) {
		delete pTexture;
	}
	m_blackouts.clear();
}

void Terrain::Initialize()
{
	//山のモデル
	m_pMountain_01 = g_Engine->AddModel(MODEL_MOUNTAIN_01);
	m_pMountain_01->m_position = XMFLOAT3(20.0f, 10.0f, 150.0f);
	m_pMountain_01->m_scale = XMFLOAT3(2.0f, 2.0f, 2.0f);
	m_pMountain_01->GetMesh(0)->pMaterial = g_Engine->GetMaterialManager()->AddMaterial("Mountain01");
	m_pMountain_01->GetMesh(0)->pMaterial->SetMainTexture(TEXTURE_MOUNTAIN_01);

	//岩のモデル
	m_pCliff_01 = g_Engine->AddModel(MODEL_CLIFF_01);
	m_pCliff_01->AddRotationX(180.0f);
	m_pCliff_01->m_position = XMFLOAT3(7.5f, 0.0f, -2.5f);
	m_pCliff_01->m_scale = XMFLOAT3(0.005f, 0.005f, 0.005f);
	m_pCliff_01->GetMesh(0)->pMaterial = g_Engine->GetMaterialManager()->AddMaterial("Cliff01", ShaderKinds::TerrainShader);
	m_pCliff_01->GetMesh(0)->pMaterial->SetMainTexture(TEXTURE_ROCK_01);
	m_pCliff_01->GetMesh(0)->pMaterial->SetNormalMap(TEXTURE_GRASS_01);

	m_pCliff_02 = g_Engine->AddModel(MODEL_CLIFF_01);
	m_pCliff_02->AddRotationX(90.0f);
	m_pCliff_02->m_position = XMFLOAT3(-15.0f, -1.5f, 10.0f);
	m_pCliff_02->m_scale = XMFLOAT3(0.0035f, 0.0035f, 0.0035f);
	m_pCliff_02->GetMesh(0)->pMaterial = g_Engine->GetMaterialManager()->AddMaterial("Cliff01", ShaderKinds::TerrainShader);

	//地面
	m_pGround = g_Engine->AddModel(MODEL_BOX);
	m_pGround->m_scale = XMFLOAT3(50.0f, 0.1f, 50.0f);
	m_pGround->m_position.y = -0.1f;
	m_pGround->GetMesh(0)->pMaterial = g_Engine->GetMaterialManager()->AddMaterial("Cliff01", ShaderKinds::TerrainShader);
	m_pGround->GetMesh(0)->pMaterial->m_diffuseColor = XMFLOAT4(1.45f, 1.45f, 1.45f, 1.0f);

	//扇風機 (アニメーション付き)
	m_pWing_01 = g_Engine->AddModel(MODEL_WING_01);
	m_pWing_01->m_position = XMFLOAT3(5.0f, 0.1f, 10.0f);
	m_pWing_01->m_scale = XMFLOAT3(0.75f, 0.75f, 0.75f);

	//扇風機のサウンド
	EmitterRef emitter = g_Engine->GetWwiseSoundSystem()->Add_Emitter();
	emitter->position = m_pWing_01->m_position;
	emitter->front = m_pWing_01->GetForward();
	emitter->top = m_pWing_01->GetUp();
	g_Engine->GetWwiseSoundSystem()->Set_Emitter(emitter);
	g_Engine->GetWwiseSoundSystem()->Play(KeyString::SOUND_SE_GAME_WIND, emitter);

	//草のモデル
	int maxGrassCount = 80;
	for (int i = 0; i < maxGrassCount; i++) {
		for (int j = 0; j < maxGrassCount; j++) {
			Model* pGrass = g_Engine->AddModel(MODEL_GRASS_01);
			//縦横ぎっしり敷き詰める
			pGrass->m_position = XMFLOAT3(0.5f * (i - maxGrassCount / 2) + 1.0f, 0.0f, 0.5f * (j - maxGrassCount / 2) + 5.0f);
			pGrass->SetRotation(0.0f, rand() % 31600 / 100.0f, 0.0f);
			pGrass->m_scale = XMFLOAT3(0.025f + (rand() % 100 - 50) / 3500.0f, 0.025f + (rand() % 100 - 50) / 2500.0f, 0.025f + (rand() % 100 - 50) / 3000.0f);
			pGrass->m_bDrawShadow = false;
			bool bExistMaterial;
			pGrass->GetMesh(0)->pMaterial = g_Engine->GetMaterialManager()->AddMaterial("Grass", bExistMaterial, ShaderKinds::GrassShader);
			if (!bExistMaterial) {
				pGrass->GetMesh(0)->pMaterial->SetMainTexture(TEXTURE_GRASS_ALPHA_01);
				pGrass->GetMesh(0)->pMaterial->SetNormalMap(TEXTURE_GRASS_NOISE_01);
			}
			pGrass->GetMesh(0)->pMaterial->m_ambientColor = XMFLOAT4(0.3f, 0.3f, 0.6f, 1.0f);
			m_pGrasses.push_back(pGrass);
		}
	}

	//ブラックアウト画像
	m_pBlackout = g_Engine->AddPrimitiveQuad();
	m_pBlackout->m_scale = XMFLOAT3(1.92f, 1.0f, 1.08f);
	m_pBlackout->m_position = XMFLOAT3(0.0f, 2.0f, 10.0f);
	m_pBlackout->GetMesh(0)->pMaterial = g_Engine->GetMaterialManager()->AddMaterial("Blackout", ShaderKinds::PrimitiveShader);

	//ブラックアウトをロード
	HCSReadFile hcsReadFile;
	hcsReadFile.Open(BLACKOUT_HCS_PATH);
	for (UINT i = 0; i < hcsReadFile.GetFileCount(); i++) {
		size_t size;
		char* pBuffer = hcsReadFile.GetFile(BLACKOUT_PATH + std::to_string(i) + ".png", &size);
		//Texture2D* pTexture = Texture2D::Get(BLACKOUT_PATH + std::to_string(i) + ".png");
		Texture2D* pTexture = Texture2D::Get(pBuffer, size);
		m_blackouts.push_back(pTexture);
		delete pBuffer;
	}

	//球体のエフェクト
	m_pEffectPlanet = g_Engine->GetEffectManager()->CreateEffect("Resource\\Effects\\aura.efk");
	m_pEffectPlanet->SetScale(XMFLOAT3(35.0f, 35.0f, 35.0f));
	m_pHandlePlanet = m_pEffectPlanet->Play(XMFLOAT3(-125.0f, 125.0f, 175.0f));
	m_pHandlePlanet->SetSpeed(0.5f);

	//ダストエフェクト
	m_pEffectDust = g_Engine->GetEffectManager()->CreateEffect("Resource\\Effects\\Dust.efkefc");
	m_pEffectDust->Play(XMFLOAT3(0.0f, 2.5f, 0.0f));
	m_pEffectDust->SetHiddenDistance(40.0f);

	//スカイボックス
	g_Engine->GetSkyBox()->SetSkyTexture("Resource\\Texture\\SkyBox.dds");
}

void Terrain::Update()
{
	float adjustTime = static_cast<float>(m_nowBlackoutCount % 3 + 1);
	m_blackoutTime += g_Engine->GetFrameTime() / adjustTime;

	if (m_blackoutTime >= BLACKOUT_FRAME_TIME && m_nowBlackoutFrame < static_cast<int>(m_blackouts.size()) - 1) {
		m_blackoutTime -= BLACKOUT_FRAME_TIME;
		m_nowBlackoutFrame++;
		m_pBlackout->GetMesh(0)->pMaterial->SetMainTexture(m_blackouts[m_nowBlackoutFrame]->Resource());

		if (m_nowBlackoutFrame >= m_blackouts.size() - 1) {
			m_blackoutTime = 0.0f;
		}
	}
	else if (m_blackoutTime >= BLACKOUT_WAIT_TIME / adjustTime) {
		m_blackoutTime = 0.0f;
		m_nowBlackoutFrame = -1;
		m_nowBlackoutCount++;
	}

	//扇風機のアニメーション
	m_pWing_01->Update();
}
