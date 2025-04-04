#include "SkyBox.h"

using namespace DirectX;

//立方体の頂点データ（大きな立方体）
const SkyboxVertex SkyBox::skyboxVertices[] =
{
    { {-1.0f,  1.0f, -1.0f} }, // 0: 左上前
    { { 1.0f,  1.0f, -1.0f} }, // 1: 右上前
    { {-1.0f, -1.0f, -1.0f} }, // 2: 左下前
    { { 1.0f, -1.0f, -1.0f} }, // 3: 右下前
    { {-1.0f,  1.0f,  1.0f} }, // 4: 左上奥
    { { 1.0f,  1.0f,  1.0f} }, // 5: 右上奥
    { {-1.0f, -1.0f,  1.0f} }, // 6: 左下奥
    { { 1.0f, -1.0f,  1.0f} }, // 7: 右下奥
};
//インデックスデータ（スカイボックス用）
const uint16_t SkyBox::skyboxIndices[] =
{
    // 前面
    0, 1, 2,
    2, 1, 3,
    // 背面
    5, 4, 7,
    7, 4, 6,
    // 左面
    4, 0, 6,
    6, 0, 2,
    // 右面
    1, 5, 3,
    3, 5, 7,
    // 上面
    4, 5, 0,
    0, 5, 1,
    // 下面
    2, 3, 6,
    6, 3, 7,
};

SkyBox::SkyBox()
    : m_pDevice(nullptr)
    , m_pCommandList(nullptr)
    , m_pCamera(nullptr)
    , m_pMaterialManager(nullptr)
    , m_pRootSignature(nullptr)
    , m_pPipelineState(nullptr)
    , m_pSkyTexture2D(nullptr)
    , m_vertexBufferView(D3D12_VERTEX_BUFFER_VIEW())
    , m_indexBufferView(D3D12_INDEX_BUFFER_VIEW())
{
}

SkyBox::~SkyBox()
{
    if (m_pRootSignature) {
        delete m_pRootSignature;
        m_pRootSignature = nullptr;
    }
    if (m_pPipelineState) {
        delete m_pPipelineState;
        m_pPipelineState = nullptr;
    }

    if (m_pSkyTexture2D) {
        delete m_pSkyTexture2D;
        m_pSkyTexture2D = nullptr;
    }
}

void SkyBox::Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, Camera* pCamera, MaterialManager* pMaterialManager)
{
    m_pDevice = pDevice;
    m_pCommandList = pCommandList;
    m_pCamera = pCamera;
    m_pMaterialManager = pMaterialManager;

    m_pRootSignature = new RootSignature(pDevice, ShaderKinds::SkyBoxShader);
    m_pPipelineState = new PipelineState(pDevice, m_pRootSignature);

    //頂点シェーダー(b0)用のリソースを作成
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    CD3DX12_RESOURCE_DESC constantData = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SkyBoxContentBuffer) + 255) & ~255);
    HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &constantData, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_projBuffer));

    if (FAILED(hr)) {
        printf("コンスタントバッファの生成に失敗:エラーコード%d\n", hr);
    }

    CreateMesh();
}

void SkyBox::SetSkyTexture(std::string ddsFile)
{
    if (m_pSkyTexture2D) {
        delete m_pSkyTexture2D;
        m_pSkyTexture2D = nullptr;
    }

    m_pSkyTexture2D = Texture2D::Get(ddsFile);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_pSkyTexture2D->Resource()->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = 1;

	m_pMaterialManager->GetDescriptorHeap()->SetResource(SKYBOX_HEAP_INDEX, m_pSkyTexture2D->Resource(), srvDesc);

    D3D12_RESOURCE_DESC texDesc = m_pSkyTexture2D->Resource()->GetDesc();
    if (texDesc.DepthOrArraySize != 6 || texDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
        printf("キューブマップテクスチャをロードできませんでした。\n");
    }
}

void SkyBox::LateUpdate()
{
    if (!m_pSkyTexture2D) {
        return;
    }

	//カメラのビュー行列から平行移動成分を除去
    SkyBoxContentBuffer buffer{};
	XMMATRIX viewMatrix = m_pCamera->m_viewMatrix;
    viewMatrix.r[3].m128_f32[0] = 0.0f;
    viewMatrix.r[3].m128_f32[1] = 0.0f;
    viewMatrix.r[3].m128_f32[2] = 0.0f;

	//VP行列を更新
    buffer.viewProj = XMMatrixMultiply(viewMatrix, m_pCamera->m_projMatrix);

	//定数バッファにデータを書き込む
	void* p0;
	HRESULT hr = m_projBuffer->Map(0, nullptr, &p0);
	if (p0) {
		memcpy(p0, &buffer, sizeof(SkyBoxContentBuffer));
		m_projBuffer->Unmap(0, nullptr);
	}
}

void SkyBox::Draw()
{
    if (!m_pSkyTexture2D) {
        return;
    }

	m_pCommandList->SetGraphicsRootSignature(m_pRootSignature->GetRootSignature());   //ルートシグネチャを設定
	m_pCommandList->SetPipelineState(m_pPipelineState->GetPipelineState());           //パイプラインステートを設定

	m_pCommandList->SetGraphicsRootConstantBufferView(0, m_projBuffer->GetGPUVirtualAddress()); //モデルの位置関係を送信

	m_pCommandList->SetGraphicsRootDescriptorTable(1, m_pMaterialManager->GetDescriptorHeap()->GetGpuDescriptorHandle(SKYBOX_HEAP_INDEX));        //頂点シェーダーのシェイプキー

    m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);    //3角ポリゴンのみ
    m_pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);                  //頂点情報を送信
	m_pCommandList->IASetIndexBuffer(&m_indexBufferView);                           //インデックス情報を送信

	m_pCommandList->DrawIndexedInstanced(std::size(skyboxIndices), 1, 0, 0, 0);     //描画
}

void SkyBox::CreateMesh()
{
    CreateVertexBuffer();
    CreateIndexBuffer();
}

//頂点バッファの作成
void SkyBox::CreateVertexBuffer()
{
    //ヒープ設定
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    //頂点バッファのリソース
    const UINT vertexBufferSize = static_cast<UINT>(std::size(skyboxVertices) * sizeof(SkyboxVertex));
    CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

    //デバイスで作成
    HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertexBuffer));

    if (FAILED(hr)) {
        printf("頂点バッファの生成に失敗しました。%1xl\n", hr);
    }

    //頂点データをGPUに送信
    void* vertexDataBegin;
    m_vertexBuffer->Map(0, nullptr, &vertexDataBegin);
    memcpy(vertexDataBegin, skyboxVertices, vertexBufferSize);
    m_vertexBuffer->Unmap(0, nullptr);

    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(SkyboxVertex);
    m_vertexBufferView.SizeInBytes = vertexBufferSize;
}

//インデックスバッファの作成
void SkyBox::CreateIndexBuffer()
{
    //ヒープ設定
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    //インデックスバッファの作成
    const UINT indexBufferSize = static_cast<UINT>(std::size(skyboxIndices) * sizeof(uint16_t));

    //インデックスバッファのリソース
    CD3DX12_RESOURCE_DESC indexBuffer = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

    HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &indexBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indexBuffer));

    if (FAILED(hr)) {
        printf("インデックスバッファの生成に失敗しました。\n");
    }

    //インデックスデータをGPUに送信
    void* indexDataBegin;
    m_indexBuffer->Map(0, nullptr, &indexDataBegin);
    memcpy(indexDataBegin, skyboxIndices, indexBufferSize);
    m_indexBuffer->Unmap(0, nullptr);

    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    m_indexBufferView.SizeInBytes = indexBufferSize;
}
