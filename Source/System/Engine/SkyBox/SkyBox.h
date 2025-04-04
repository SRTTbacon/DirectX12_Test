#pragma once

#include <string>
#include "..\\..\\ComPtr.h"
#include "..\\..\\Main\\Utility.h"
#include "..\\Camera\\Camera.h"
#include "..\\Model\\ModelManager.h"
#include "..\\Core\\DDSTextureLoader\\DDSTextureLoader.h"
#include <DirectXTex.h>

struct SkyboxVertex
{
    DirectX::XMFLOAT3 position;
};

class SkyBox
{
public:
    SkyBox();
    ~SkyBox();

    void Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, Camera* pCamera, MaterialManager* pMaterialManager);

	void SetSkyTexture(std::string ddsFile);

    void LateUpdate();
    
    void Draw();

private:
    struct SkyBoxContentBuffer
    {
        DirectX::XMMATRIX viewProj;
    };

    static const SkyboxVertex skyboxVertices[];
    static const uint16_t skyboxIndices[];

	//DDSデータを格納する変数
	std::unique_ptr<uint8_t[]> m_ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> m_subresources;
    Texture2D* m_pSkyTexture2D;
    ComPtr<ID3D12Resource> m_projBuffer;        //コンスタントバッファ
    ComPtr<ID3D12Resource> m_vertexBuffer;
    ComPtr<ID3D12Resource> m_indexBuffer;

    ID3D12Device* m_pDevice;
    ID3D12GraphicsCommandList* m_pCommandList;

    RootSignature* m_pRootSignature;            //ルートシグネチャ
    PipelineState* m_pPipelineState;            //パイプラインステート

    Camera* m_pCamera;
    MaterialManager* m_pMaterialManager;

    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

    void CreateMesh();
    //頂点バッファの作成
    void CreateVertexBuffer();
    //インデックスバッファの作成
    void CreateIndexBuffer();
};
