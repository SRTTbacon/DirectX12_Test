#pragma once
#include "Material.h"

class MaterialManager
{
public:
	MaterialManager();
	~MaterialManager();

	//�}�e���A���}�l�[�W���[��������
	//�ȉ��̊֐������s����O�ɌĂяo���K�v������܂�
	void Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, DirectionalLight* pDirectionalLight, ID3D12Resource* pShadowMap);

	//�V���Ƀ}�e���A�����쐬
	//���Ƀ}�e���A���������݂���ꍇ�͑��݂���}�e���A����Ԃ�
	//���� : std::string �}�e���A����, (OUT) bool& �}�e���A�������ɑ��݂��邩�ǂ���, ShaderKinds �V�F�[�_�[�̎��
	//�߂�l : �쐬�܂��͊��ɑ��݂����ꍇ�͏��������}�e���A���N���X�̃|�C���^
	Material* AddMaterial(std::string materialName, OUT bool& bExist, ShaderKinds shaderType = ShaderKinds::PrimitiveShader);

	//�V���Ƀ}�e���A�����쐬
	//���Ƀ}�e���A���������݂���ꍇ�͑��݂���}�e���A����Ԃ�
	//���� : std::string �}�e���A����, ShaderKinds �V�F�[�_�[�̎��
	//�߂�l : �쐬�܂��͊��ɑ��݂����ꍇ�͏��������}�e���A���N���X�̃|�C���^
	Material* AddMaterial(std::string materialName, ShaderKinds shaderType = ShaderKinds::PrimitiveShader);

	//�V���ɃV�F�C�v��񂪑��݂���}�e���A�����쐬
	//���Ƀ}�e���A���������݂���ꍇ�͑��݂���}�e���A����Ԃ�
	//���� : std::string �}�e���A����, (OUT) bool& �}�e���A�������ɑ��݂��邩�ǂ���, (OUT) UINT& �V�F�C�v��񂪊i�[�����ꏊ
	//�߂�l : �쐬�܂��͊��ɑ��݂����ꍇ�͏��������}�e���A���N���X�̃|�C���^
	Material* AddMaterialWithShapeData(std::string materialName, OUT bool& bExist, OUT UINT& index);

	//�V���ɃV�F�C�v��񂪑��݂���}�e���A�����쐬
	//���Ƀ}�e���A���������݂���ꍇ�͑��݂���}�e���A����Ԃ�
	//���� : std::string �}�e���A����, (OUT) UINT& �V�F�C�v��񂪊i�[�����ꏊ
	//�߂�l : �쐬�܂��͊��ɑ��݂����ꍇ�͏��������}�e���A���N���X�̃|�C���^
	Material* AddMaterialWithShapeData(std::string materialName, OUT UINT& index);

	//�}�e���A���������݂��邩�ǂ���
	bool IsExistMaterialName(std::string materialName) const;

	//���ׂẴ}�e���A���Ŏg�p����f�B�X�N���v�^�q�[�v���R�}���h���X�g�ɃZ�b�g
	void SetHeap();

public:
	inline DescriptorHeap* GetDescriptorHeap() { return &m_descriptorHeap; }

private:
	struct Pipeline
	{
		RootSignature* pRootSignature;
		PipelineState* pPipelineState;
	};

	std::unordered_map<ShaderKinds, Pipeline> m_pipelines;	//�V�F�[�_�[�ʂ̃��[�g�V�O�l�`���ƃp�C�v���C���X�e�[�g
	std::unordered_map<std::string, Material*> m_materials;	//���ׂẴ}�e���A�����Ǘ�(Key:�}�e���A����, Value:�}�e���A���̃|�C���^)

	ID3D12Device* m_pDevice;                    //�G���W���̃f�o�C�X
	ID3D12GraphicsCommandList* m_pCommandList;  //�G���W���̃R�}���h���X�g

	DirectionalLight* m_pDirectionalLight;      //����
	DescriptorHeap m_descriptorHeap;			//���ׂẴ}�e���A���̃e�N�X�`��������f�B�X�N���v�^�q�[�v

	ID3D12Resource* m_pShadowMap;				//�V���h�E�}�b�v�̃��\�[�X

	Texture2D* m_pWhiteTexture;		//���F�̃e�N�X�`��
	Texture2D* m_pNormalTexture;	//�m�[�}���}�b�v�p�̃e�N�X�`��(�e����^���Ȃ�)

	std::vector<bool> m_bFreeShapeDataIDs;	//�V�F�C�v�����i�[����C���f�b�N�X���g�p�\���ǂ��� (true�̏ꍇ�͎g�p���Ă��Ȃ�)
	std::vector<bool> m_bFreeMaterialIDs;	//�}�e���A��ID���g�p�\���ǂ��� (true�̏ꍇ�͎g�p���Ă��Ȃ�)
	UINT m_nextShapeDataID;					//���Ɋi�[����ۂɎg�p����ID
	UINT m_nextMaterialID;					//���Ɋi�[����ۂɎg�p����ID
};