#pragma once
#include "Material.h"

class MaterialManager
{
public:
	MaterialManager();
	~MaterialManager();

	//マテリアルマネージャーを初期化
	//以下の関数を実行する前に呼び出す必要があります
	void Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, DirectionalLight* pDirectionalLight, ID3D12Resource* pShadowMap);

	//新たにマテリアルを作成
	//既にマテリアル名が存在する場合は存在するマテリアルを返す
	//引数 : std::string マテリアル名, (OUT) bool& マテリアルが既に存在するかどうか, ShaderKinds シェーダーの種類
	//戻り値 : 作成または既に存在した場合は所得したマテリアルクラスのポインタ
	Material* AddMaterial(std::string materialName, OUT bool& bExist, ShaderKinds shaderType = ShaderKinds::PrimitiveShader);

	//新たにマテリアルを作成
	//既にマテリアル名が存在する場合は存在するマテリアルを返す
	//引数 : std::string マテリアル名, ShaderKinds シェーダーの種類
	//戻り値 : 作成または既に存在した場合は所得したマテリアルクラスのポインタ
	Material* AddMaterial(std::string materialName, ShaderKinds shaderType = ShaderKinds::PrimitiveShader);

	//新たにシェイプ情報が存在するマテリアルを作成
	//既にマテリアル名が存在する場合は存在するマテリアルを返す
	//引数 : std::string マテリアル名, (OUT) bool& マテリアルが既に存在するかどうか, (OUT) UINT& シェイプ情報が格納される場所
	//戻り値 : 作成または既に存在した場合は所得したマテリアルクラスのポインタ
	Material* AddMaterialWithShapeData(std::string materialName, OUT bool& bExist, OUT UINT& index);

	//新たにシェイプ情報が存在するマテリアルを作成
	//既にマテリアル名が存在する場合は存在するマテリアルを返す
	//引数 : std::string マテリアル名, (OUT) UINT& シェイプ情報が格納される場所
	//戻り値 : 作成または既に存在した場合は所得したマテリアルクラスのポインタ
	Material* AddMaterialWithShapeData(std::string materialName, OUT UINT& index);

	//マテリアル名が存在するかどうか
	bool IsExistMaterialName(std::string materialName) const;

	//すべてのマテリアルで使用するディスクリプタヒープをコマンドリストにセット
	void SetHeap();

public:
	inline DescriptorHeap* GetDescriptorHeap() { return &m_descriptorHeap; }

private:
	struct Pipeline
	{
		RootSignature* pRootSignature;
		PipelineState* pPipelineState;
	};

	std::unordered_map<ShaderKinds, Pipeline> m_pipelines;	//シェーダー別のルートシグネチャとパイプラインステート
	std::unordered_map<std::string, Material*> m_materials;	//すべてのマテリアルを管理(Key:マテリアル名, Value:マテリアルのポインタ)

	ID3D12Device* m_pDevice;                    //エンジンのデバイス
	ID3D12GraphicsCommandList* m_pCommandList;  //エンジンのコマンドリスト

	DirectionalLight* m_pDirectionalLight;      //環境光
	DescriptorHeap m_descriptorHeap;			//すべてのマテリアルのテクスチャが入るディスクリプタヒープ

	ID3D12Resource* m_pShadowMap;				//シャドウマップのリソース

	Texture2D* m_pWhiteTexture;		//白色のテクスチャ
	Texture2D* m_pNormalTexture;	//ノーマルマップ用のテクスチャ(影響を与えない)

	std::vector<bool> m_bFreeShapeDataIDs;	//シェイプ情報を格納するインデックスが使用可能かどうか (trueの場合は使用していない)
	std::vector<bool> m_bFreeMaterialIDs;	//マテリアルIDが使用可能かどうか (trueの場合は使用していない)
	UINT m_nextShapeDataID;					//次に格納する際に使用するID
	UINT m_nextMaterialID;					//次に格納する際に使用するID
};