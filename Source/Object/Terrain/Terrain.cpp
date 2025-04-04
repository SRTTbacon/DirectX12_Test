#include "Terrain.h"
#include "..\\..\\System\\Engine\\Model\\Convert\\ConvertFromFBX.h"

const std::string Terrain::MODEL_BOX = "Resource\\Model\\Plane.fbx";
const std::string Terrain::MODEL_MOUNTAIN_01 = "Resource\\Model\\Terrains\\Mountains\\SM_Terrain_02.hcs";
const std::string Terrain::MODEL_CLIFF_01 = "Resource\\Model\\Terrains\\Cliffs\\Cliff04.hcs";
const std::string Terrain::MODEL_WING_01 = "Resource\\Model\\î•—‹@.hcs";
const std::string Terrain::MODEL_GRASS_01 = "Resource\\Model\\Terrains\\Grass\\Grass01.hcs";
const std::string Terrain::TEXTURE_MOUNTAIN_01 = "Resource\\Model\\Terrains\\Textures\\Mountain_02_C.png";
const std::string Terrain::TEXTURE_ROCK_01 = "Resource\\Model\\Terrains\\Textures\\Cliff04_a.png";
const std::string Terrain::TEXTURE_GRASS_01 = "Resource\\Model\\Terrains\\Textures\\Grass01_a.png";
const std::string Terrain::TEXTURE_GRASS_ALPHA_01 = "Resource\\Model\\Terrains\\Grass\\Grass01.png";

Terrain::Terrain()
	: m_pGround(nullptr)
	, m_pMountain_01(nullptr)
	, m_pCliff_01(nullptr)
	, m_pCliff_02(nullptr)
	, m_pWing_01(nullptr)
	, m_pEffectPlanet(nullptr)
	, m_pEffectDust(nullptr)
	, m_pHandlePlanet(nullptr)
{
}

void Terrain::Initialize()
{
	//’n–Ê
	m_pGround = g_Engine->AddModel(MODEL_BOX);
	m_pGround->m_scale = XMFLOAT3(50.0f, 0.1f, 50.0f);
	m_pGround->m_position.y = -0.1f;
	
	m_pMountain_01 = g_Engine->AddModel(MODEL_MOUNTAIN_01);
	m_pMountain_01->m_position = XMFLOAT3(20.0f, 10.0f, 150.0f);
	m_pMountain_01->m_scale = XMFLOAT3(2.0f, 2.0f, 2.0f);
	m_pMountain_01->GetMesh(0)->pMaterial = g_Engine->GetMaterialManager()->AddMaterial("Mountain01");
	m_pMountain_01->GetMesh(0)->pMaterial->SetMainTexture(TEXTURE_MOUNTAIN_01);

	m_pCliff_01 = g_Engine->AddModel(MODEL_CLIFF_01);
	m_pCliff_01->AddRotationX(180.0f);
	m_pCliff_01->m_position = XMFLOAT3(7.5f, 0.0f, -2.5f);
	m_pCliff_01->m_scale = XMFLOAT3(0.005f, 0.005f, 0.005f);
	m_pCliff_01->GetMesh(0)->pMaterial = g_Engine->GetMaterialManager()->AddMaterial("Cliff01", ShaderKinds::TerrainShader);
	m_pCliff_01->GetMesh(0)->pMaterial->SetMainTexture(TEXTURE_ROCK_01);
	m_pCliff_01->GetMesh(0)->pMaterial->SetNormalMap(TEXTURE_GRASS_01);

	m_pCliff_02 = g_Engine->AddModel(MODEL_CLIFF_01);
	m_pCliff_02->AddRotationX(180.0f);
	m_pCliff_02->m_position = XMFLOAT3(-10.0f, -3.0f, 10.0f);
	m_pCliff_02->m_scale = XMFLOAT3(0.0035f, 0.0035f, 0.0035f);
	m_pCliff_02->GetMesh(0)->pMaterial = g_Engine->GetMaterialManager()->AddMaterial("Cliff01", ShaderKinds::TerrainShader);
	//m_pCliff_02->GetMesh(0)->pMaterial->SetMainTexture(TEXTURE_ROCK_01);
	//m_pCliff_02->GetMesh(0)->pMaterial->SetNormalMap(TEXTURE_GRASS_01);

	m_pWing_01 = g_Engine->AddModel(MODEL_WING_01);
	m_pWing_01->m_position = XMFLOAT3(5.0f, 0.1f, 10.0f);
	m_pWing_01->m_scale = XMFLOAT3(0.75f, 0.75f, 0.75f);

	int maxGrassCount = 10;
	for (int i = 0; i < maxGrassCount; i++) {
		for (int j = 0; j < maxGrassCount; j++) {
			Model* pGrass = g_Engine->AddModel(MODEL_GRASS_01);
			pGrass->m_position = XMFLOAT3(0.5f * (i - maxGrassCount / 2), 0.0f, 0.5f * (j - maxGrassCount / 2));
			pGrass->SetRotation(0.0f, rand() % 31600 / 100.0f, 0.0f);
			pGrass->m_scale = XMFLOAT3(0.025f + (rand() % 100 - 50) / 3500.0f, 0.025f + (rand() % 100 - 50) / 2500.0f, 0.025f + (rand() % 100 - 50) / 3000.0f);
			pGrass->m_bDrawShadow = false;
			bool bExistMaterial;
			pGrass->GetMesh(0)->pMaterial = g_Engine->GetMaterialManager()->AddMaterial("Grass", bExistMaterial, ShaderKinds::GrassShader);
			if (!bExistMaterial) {
				pGrass->GetMesh(0)->pMaterial->SetMainTexture(TEXTURE_GRASS_ALPHA_01);
			}
			m_pGrasses.push_back(pGrass);
		}
	}

	m_pEffectPlanet = g_Engine->GetEffectManager()->CreateEffect("Resource\\Effects\\aura.efk");
	m_pEffectPlanet->SetScale(XMFLOAT3(35.0f, 35.0f, 35.0f));
	m_pHandlePlanet = m_pEffectPlanet->Play(XMFLOAT3(-125.0f, 125.0f, 175.0f));
	m_pHandlePlanet->SetSpeed(0.5f);

	m_pEffectDust = g_Engine->GetEffectManager()->CreateEffect("Resource\\Effects\\Dust.efkefc");
	m_pEffectDust->Play(XMFLOAT3(0.0f, 2.5f, 0.0f));
	m_pEffectDust->SetHiddenDistance(40.0f);

	g_Engine->GetSkyBox()->SetSkyTexture("Resource\\Texture\\SkyBox.dds");
}

void Terrain::Update()
{
	if (g_Engine->GetKeyState(DIK_UP)) {
		m_pMountain_01->GetMesh(0)->m_position.z -= 0.1f;
	}
	if (g_Engine->GetKeyState(DIK_DOWN)) {
		m_pMountain_01->GetMesh(0)->m_position.z += 0.1f;
	}
	if (g_Engine->GetKeyState(DIK_LEFT)) {
		m_pMountain_01->GetMesh(0)->m_position.x -= 0.1f;
	}
	if (g_Engine->GetKeyState(DIK_RIGHT)) {
		m_pMountain_01->GetMesh(0)->m_position.x += 0.1f;
	}
	if (g_Engine->GetKeyState(DIK_3)) {
		m_pCliff_01->m_position.y -= 0.1f;
	}
	if (g_Engine->GetKeyState(DIK_4)) {
		m_pCliff_01->m_position.y += 0.1f;
	}
	if (g_Engine->GetKeyState(DIK_5)) {
		m_pCliff_01->m_rotation.x -= 2.0f;
	}
	if (g_Engine->GetKeyState(DIK_6)) {
		m_pCliff_01->m_rotation.x += 2.0f;
	}
	if (g_Engine->GetKeyStateSync(DIK_C)) {
		m_pHandlePlanet->m_bDraw = !m_pHandlePlanet->m_bDraw;

		ConvertFromFBX convert;
		//printf("%d\n", convert.ConvertFromModel(m_pMountain_01->GetModelFilePath(), "Resource\\Model\\Terrains\\Mountains\\SM_Terrain_02.hcs"));
	}

	m_pWing_01->Update();
}
