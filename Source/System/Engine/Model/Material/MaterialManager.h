#pragma once
#include "Material.h"

class MaterialManager
{
public:
	MaterialManager();
	~MaterialManager();

	void Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, DirectionalLight* pDirectionalLight, ID3D12Resource* pShadowMap);

	//新たにマテリアルを作成
	//既にマテリアル名が存在する場合は存在するマテリアルを返す
	//引数 : std::string マテリアル名, ShaderKinds シェーダーの種類
	//戻り値 : 作成または既に存在した場合は所得したマテリアルクラスのポインタ
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

	ID3D12Device* m_pDevice;                    //エンジンのデバイス
	ID3D12GraphicsCommandList* m_pCommandList;  //エンジンのコマンドリスト

	DirectionalLight* m_pDirectionalLight;      //環境光
	DescriptorHeap m_descriptorHeap;			//すべてのマテリアルのテクスチャが入るディスクリプタヒープ

	ID3D12Resource* m_pShadowMap;

	Texture2D* m_pWhiteTexture;		//白色のテクスチャ
	Texture2D* m_pNormalTexture;	//ノーマルマップ用のテクスチャ(影響を与えない)

	std::vector<bool> m_bFreeShapeDataIDs;
	std::vector<bool> m_bFreeMaterialIDs;
	UINT m_nextMaterialID;
	UINT m_nextShapeDataID;
};