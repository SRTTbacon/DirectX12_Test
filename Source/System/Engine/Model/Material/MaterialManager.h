#pragma once
#include "Material.h"

class MaterialManager
{
public:
	MaterialManager();
	~MaterialManager();

	void Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, DirectionalLight* pDirectionalLight, ID3D12Resource* pShadowMap);

	//�V���Ƀ}�e���A�����쐬
	//���Ƀ}�e���A���������݂���ꍇ�͑��݂���}�e���A����Ԃ�
	//���� : std::string �}�e���A����, ShaderKinds �V�F�[�_�[�̎��
	//�߂�l : �쐬�܂��͊��ɑ��݂����ꍇ�͏��������}�e���A���N���X�̃|�C���^
	Material* AddMaterial(std::string materialName, OUT bool& bExist, ShaderKinds shaderType = ShaderKinds::PrimitiveShader);
	Material* AddMaterial(std::string materialName, ShaderKinds shaderType = ShaderKinds::PrimitiveShader);
	Material* AddMaterialWithShapeData(std::string materialName, OUT bool& bExist, OUT UINT& index);
	Material* AddMaterialWithShapeData(std::string materialName, OUT UINT& index);

	void SetHeap();

private:
	struct Pipeline
	{
		RootSignature* pRootSignature;
		PipelineState* pPipelineState;
	};

	std::unordered_map<ShaderKinds, Pipeline> m_pipelines;
	std::unordered_map<std::string, Material*> m_materials;

	ID3D12Device* m_pDevice;                    //�G���W���̃f�o�C�X
	ID3D12GraphicsCommandList* m_pCommandList;  //�G���W���̃R�}���h���X�g

	DirectionalLight* m_pDirectionalLight;      //����
	DescriptorHeap m_descriptorHeap;			//���ׂẴ}�e���A���̃e�N�X�`��������f�B�X�N���v�^�q�[�v

	ID3D12Resource* m_pShadowMap;

	Texture2D* m_pWhiteTexture;		//���F�̃e�N�X�`��
	Texture2D* m_pNormalTexture;	//�m�[�}���}�b�v�p�̃e�N�X�`��(�e����^���Ȃ�)

	std::vector<bool> m_bFreeShapeDataIDs;
	std::vector<bool> m_bFreeMaterialIDs;
	UINT m_nextMaterialID;
	UINT m_nextShapeDataID;
};