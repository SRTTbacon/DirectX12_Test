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

	//�V�F�C�v���̃��\�[�X���i�[�ł��鐔���v�Z
	//���q�[�v�T�C�Y����}�e���A���ƃV���h�E�}�b�v�Ŏg�p���郊�\�[�X������������
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
	//�G���W������K�v�ȏ����󂯎��
	m_pDevice = pDevice;
	m_pCommandList = pCommandList;
	m_pDirectionalLight = pDirectionalLight;
	m_pShadowMap = pShadowMap;

	//�f�B�X�N���v�^�q�[�v���쐬
	m_descriptorHeap.Initialize(pDevice, MAX_DESCRIPTORHEAP_SIZE);
	//�V���h�E�}�b�v�̓q�[�v�̈�ԏ��߂Ɋi�[
	m_descriptorHeap.SetResource(SHADOWMAP_HEAP_INDEX, pShadowMap, SHADOWMAP_FORMAT);

	//�f�t�H���g�e�N�X�`�����쐬 (���F�e�N�X�`���A�e����^���Ȃ��m�[�}���}�b�v)
	m_pWhiteTexture = Texture2D::GetColor(1.0f, 1.0f, 1.0f);
	m_pNormalTexture = Texture2D::GetColor(0.5f, 0.5f, 1.0f);
}

Material* MaterialManager::AddMaterial(string materialName, OUT bool& bExist, ShaderKinds shaderType)
{
	//���ɓ����̃}�e���A�������݂���΂����Ԃ�
	if (m_materials.find(materialName) != m_materials.end()) {
		bExist = true;
		return m_materials[materialName];
	}

	bExist = false;

	//�}�e���A��ID������
	bool bLooping = false;
	for (UINT i = m_nextMaterialID; i < MAX_MATERIAL_COUNT; i++) {
		if (m_bFreeMaterialIDs[i]) {
			m_nextMaterialID = i;
			m_bFreeMaterialIDs[i] = false;
			break;
		}
		if (i >= MAX_MATERIAL_COUNT - 1) {
			//�}�e���A���̍ő吔�𒴂���ꍇ�̓G���[
			if (bLooping) {
				assert("�G���[:�}�e���A�����K�萔�𒴂��Ă��邽�ߍ쐬�ł��܂���B");
				break;
			}
			bLooping = true;
			i = -1;
			continue;
		}
	}

	//�}�e���A�����쐬
	Material* pNewMat = new Material(m_pDevice, m_pCommandList, m_pDirectionalLight, &m_descriptorHeap, m_nextMaterialID);
	pNewMat->Initialize(m_pWhiteTexture->Resource(), m_pNormalTexture->Resource());

	//�V�F�[�_�[�̃p�C�v���C��������������Ă��Ȃ��ꍇ�͏�����
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
	//bExist���g�p���Ȃ�
	bool temp;
	return AddMaterial(materialName, temp, shaderType);
}

Material* MaterialManager::AddMaterialWithShapeData(std::string materialName, OUT bool& bExist, OUT UINT& index)
{
	//�V�F�C�v�L�[ID������
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
			//�V�F�C�v�L�[�̃��\�[�X���ő吔�𒴂���ꍇ�̓G���[
			if (bLooping) {
				assert("�G���[:�V�F�C�v�L�[�̃��\�[�X���K�萔�𒴂��Ă��邽�߃}�e���A�����쐬�ł��܂���B");
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
	//bExist���g�p���Ȃ�
	bool temp;
	return AddMaterialWithShapeData(materialName, temp, index);
}

bool MaterialManager::IsExistMaterialName(std::string materialName) const
{
	return m_materials.find(materialName) != m_materials.end();
}

void MaterialManager::SetHeap()
{
	//�f�B�X�N���v�^�q�[�v���Z�b�g
	ID3D12DescriptorHeap* heap = m_descriptorHeap.GetHeap();
	m_pCommandList->SetDescriptorHeaps(1, &heap);
}
