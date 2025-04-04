#include "MaterialManager.h"

using namespace std;

MaterialManager::MaterialManager()
	: m_pDevice(nullptr)
	, m_pCommandList(nullptr)
	, m_pDirectionalLight(nullptr)
	, m_pShadowMap(nullptr)
	, m_pWhiteTexture(nullptr)
	, m_pNormalTexture(nullptr)
	, m_nextMaterialID(0)
	, m_nextShapeDataID(0)
{
	for (UINT i = 0; i < MAX_MATERIAL_COUNT; i++) {
		m_bFreeMaterialIDs.push_back(true);
	}

	//シェイプ情報のリソースが格納できる数を計算
	//※ヒープサイズからマテリアルとシャドウマップで使用するリソース数を引いた数
	UINT startIndex = MAX_MATERIAL_COUNT * MATERIAL_DISCRIPTOR_HEAP_SIZE + 1;
	UINT maxCount = MAX_DESCRIPTORHEAP_SIZE - startIndex;
	for (UINT i = 0; i < maxCount; i++) {
		m_bFreeShapeDataIDs.push_back(true);
	}
}

MaterialManager::~MaterialManager()
{
	if (m_pWhiteTexture) {
		delete m_pWhiteTexture;
		m_pWhiteTexture = nullptr;
	}
	if (m_pNormalTexture) {
		delete m_pNormalTexture;
		m_pNormalTexture = nullptr;
	}

	for (auto& keyValue : m_pipelines) {
		delete keyValue.second.pPipelineState;
		delete keyValue.second.pRootSignature;
	}

	for (auto& keyValue : m_materials) {
		delete keyValue.second;
	}
}

void MaterialManager::Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, DirectionalLight* pDirectionalLight, ID3D12Resource* pShadowMap)
{
	//エンジンから必要な情報を受け取る
	m_pDevice = pDevice;
	m_pCommandList = pCommandList;
	m_pDirectionalLight = pDirectionalLight;
	m_pShadowMap = pShadowMap;

	//ディスクリプタヒープを作成
	m_descriptorHeap.Initialize(pDevice, MAX_DESCRIPTORHEAP_SIZE);
	//シャドウマップはヒープの一番初めに格納
	m_descriptorHeap.SetResource(SHADOWMAP_HEAP_INDEX, pShadowMap, SHADOWMAP_FORMAT);

	//デフォルトテクスチャを作成 (白色テクスチャ、影響を与えないノーマルマップ)
	m_pWhiteTexture = Texture2D::GetColor(1.0f, 1.0f, 1.0f);
	m_pNormalTexture = Texture2D::GetColor(0.5f, 0.5f, 1.0f);
}

Material* MaterialManager::AddMaterial(string materialName, OUT bool& bExist, ShaderKinds shaderType)
{
	//既に同名のマテリアルが存在すればそれを返す
	if (m_materials.find(materialName) != m_materials.end()) {
		bExist = true;
		return m_materials[materialName];
	}

	bExist = false;

	//マテリアルIDを決定
	bool bLooping = false;
	for (UINT i = m_nextMaterialID; i < MAX_MATERIAL_COUNT; i++) {
		if (m_bFreeMaterialIDs[i]) {
			m_nextMaterialID = i;
			m_bFreeMaterialIDs[i] = false;
			break;
		}
		if (i >= MAX_MATERIAL_COUNT - 1) {
			//マテリアルの最大数を超える場合はエラー
			if (bLooping) {
				assert("エラー:マテリアルが規定数を超えているため作成できません。");
				break;
			}
			bLooping = true;
			i = -1;
			continue;
		}
	}

	//マテリアルを作成
	Material* pNewMat = new Material(m_pDevice, m_pCommandList, m_pDirectionalLight, &m_descriptorHeap, m_nextMaterialID);
	pNewMat->Initialize(m_pWhiteTexture->Resource(), m_pNormalTexture->Resource());

	//シェーダーのパイプラインが初期化されていない場合は初期化
	if (m_pipelines.find(shaderType) == m_pipelines.end()) {
		Pipeline pipeline{};
		pipeline.pRootSignature = new RootSignature(m_pDevice, shaderType);
		pipeline.pPipelineState = new PipelineState(m_pDevice, pipeline.pRootSignature);
		m_pipelines[shaderType] = pipeline;
	}

	pNewMat->SetPipeline(m_pipelines[shaderType].pRootSignature, m_pipelines[shaderType].pPipelineState, shaderType);

	m_materials[materialName] = pNewMat;

	return pNewMat;
}

Material* MaterialManager::AddMaterial(std::string materialName, ShaderKinds shaderType)
{
	//bExistを使用しない
	bool temp;
	return AddMaterial(materialName, temp, shaderType);
}

Material* MaterialManager::AddMaterialWithShapeData(std::string materialName, OUT bool& bExist, OUT UINT& index)
{
	//シェイプキーIDを決定
	bool bLooping = false;
	UINT startIndex = MAX_MATERIAL_COUNT * MATERIAL_DISCRIPTOR_HEAP_SIZE + 1;
	UINT maxCount = MAX_DESCRIPTORHEAP_SIZE - startIndex;
	for (UINT i = m_nextShapeDataID; i < maxCount; i++) {
		if (m_bFreeShapeDataIDs[i]) {
			m_nextShapeDataID = i;
			m_bFreeShapeDataIDs[i] = false;
			break;
		}
		if (i >= maxCount - 1) {
			//シェイプキーのリソースが最大数を超える場合はエラー
			if (bLooping) {
				assert("エラー:シェイプキーのリソースが規定数を超えているためマテリアルを作成できません。");
				break;
			}
			bLooping = true;
			i = -1;
			continue;
		}
	}

	index = startIndex + m_nextShapeDataID;

	return AddMaterial(materialName, bExist, ShaderKinds::BoneShader);
}

Material* MaterialManager::AddMaterialWithShapeData(std::string materialName, OUT UINT& index)
{
	//bExistを使用しない
	bool temp;
	return AddMaterialWithShapeData(materialName, temp, index);
}

bool MaterialManager::IsExistMaterialName(std::string materialName) const
{
	return m_materials.find(materialName) != m_materials.end();
}

void MaterialManager::SetHeap()
{
	//ディスクリプタヒープをセット
	ID3D12DescriptorHeap* heap = m_descriptorHeap.GetHeap();
	m_pCommandList->SetDescriptorHeaps(1, &heap);
}
