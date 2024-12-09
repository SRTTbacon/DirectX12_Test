#include "Model.h"
#include <d3dx12.h>
#include <stdexcept>
#include "..\\..\\Main\\Main.h"

Model::Model(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const Camera* pCamera, DirectionalLight* pDirectionalLight, ID3D12Resource* pShadowMapBuffer)
    : m_modelMatrix(XMMatrixIdentity())
    , m_pCamera(pCamera)
    , m_pDevice(pDevice)
    , m_pCommandList(pCommandList)
    , m_pDirectionalLight(pDirectionalLight)
    , m_pShadowMapBuffer(pShadowMapBuffer)
    , m_pPipelineState(nullptr)
    , m_pRootSignature(nullptr)
    , m_pDescriptorHeap(nullptr)
    , m_position(XMFLOAT3(0.0f, 0.0f, 0.0f))
    , m_rotation(XMFLOAT3(0.0f, 0.0f, 0.0f))
    , m_scale(XMFLOAT3(1.0f, 1.0f, 1.0f))
    , m_depth(0.0f)
    , m_bVisible(true)
    , m_bTransparent(false)
{
}

void Model::LoadModel(const std::string fbxFile)
{
    m_fbxFile = fbxFile;

    CreateConstantBuffer();

    Assimp::Importer importer;

    //モデル読み込み時のフラグ。メッシュのポリゴンはすべて三角形にし、左手座標系に変換
    unsigned int flag = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
        aiProcess_CalcTangentSpace;
    const aiScene* scene = importer.ReadFile(fbxFile, flag);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        printf("FBXファイルをロードできませんでした。\n");
        return;
    }

    //ディスクリプタヒープを作成 (メッシュの数 + 影1つ)
    m_pDescriptorHeap = new DescriptorHeap(m_pDevice, scene->mNumMeshes * 2, ShadowSizeHigh);

    ProcessNode(scene, scene->mRootNode);

    m_pRootSignature = new RootSignature(m_pDevice, ShaderKinds::PrimitiveShader);
    m_pPipelineState = new PipelineState(m_pDevice, m_pRootSignature);
}

void Model::ProcessNode(const aiScene* scene, aiNode* node) {
    // メッシュを処理
    for (UINT i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_meshes.push_back(ProcessMesh(scene, mesh));
    }

    // 子ノードも再帰的に処理
    for (UINT i = 0; i < node->mNumChildren; i++) {
        ProcessNode(scene, node->mChildren[i]);
    }
}

Model::Mesh* Model::ProcessMesh(const aiScene* scene, aiMesh* mesh) {
    std::vector<VertexPrimitive> vertices;
    std::vector<UINT> indices;

    // 頂点の処理
    for (UINT i = 0; i < mesh->mNumVertices; i++) {
        VertexPrimitive vertex{};

        vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
        vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
        if (mesh->mTextureCoords[0]) {
            vertex.texCoords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        }
        else {
            vertex.texCoords = { 0.0f, 0.0f };
        }
        vertex.boneIDs[0] = vertex.boneIDs[1] = vertex.boneIDs[2] = vertex.boneIDs[3] = 0;
        vertex.boneWeights = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);

        vertices.push_back(vertex);
    }

    // インデックスの処理
    for (UINT i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (UINT j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    Mesh* meshData = new Mesh();

    CreateBuffer(meshData, vertices, indices);

    //meshData->materialIndex = -1;

    //マテリアルを作成
    m_pDescriptorHeap->SetMainTexture(Texture2D::GetWhite()->Resource(), m_pShadowMapBuffer);

    return meshData;
}

void Model::CreateConstantBuffer()
{
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    CD3DX12_RESOURCE_DESC constantData = CD3DX12_RESOURCE_DESC::Buffer((sizeof(ModelConstantBuffer) + 255) & ~255);
    //定数バッファをリソースとして作成
    for (int i = 0; i < FRAME_BUFFER_COUNT; i++) {
        HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &constantData,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_modelConstantBuffer[i]));

        if (FAILED(hr)) {
            printf("コンスタントバッファの生成に失敗:エラーコード%d\n", hr);
        }
        else {
            printf("コンスタントバッファを生成しました。%s\n", m_fbxFile.c_str());
        }
    }

    //ディレクショナルライトの情報
    CD3DX12_RESOURCE_DESC lightBuf = CD3DX12_RESOURCE_DESC::Buffer(sizeof(LightBuffer));
    m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &lightBuf,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_lightConstantBuffer));
}

void Model::RenderShadowMap(UINT backBufferIndex)
{
    m_pCommandList->SetGraphicsRootConstantBufferView(0, m_modelConstantBuffer[backBufferIndex]->GetGPUVirtualAddress());

    if (m_boneMatricesBuffer) {
        m_pCommandList->SetGraphicsRootConstantBufferView(1, m_boneMatricesBuffer->GetGPUVirtualAddress());   //ボーンを送信
    }

    //メッシュの深度情報をシャドウマップに描画
    for (const Mesh* pMesh : m_meshes)
    {
        pMesh->Draw(m_pCommandList); //深度のみを描画
    }
}

void Model::RenderSceneWithShadow(UINT backBufferIndex)
{
    //コマンドリストに送信
    m_pCommandList->SetGraphicsRootSignature(m_pRootSignature->GetRootSignature());   //ルートシグネチャを設定
    m_pCommandList->SetPipelineState(m_pPipelineState->GetPipelineState());           //パイプラインステートを設定

    m_pCommandList->SetGraphicsRootConstantBufferView(0, m_modelConstantBuffer[backBufferIndex]->GetGPUVirtualAddress()); //モデルの位置関係を送信
    m_pCommandList->SetGraphicsRootConstantBufferView(1, m_lightConstantBuffer->GetGPUVirtualAddress()); //ディレクショナルライトの情報を送信

    if (m_boneMatricesBuffer) {
        m_pCommandList->SetGraphicsRootConstantBufferView(3, m_boneMatricesBuffer->GetGPUVirtualAddress());   //ボーンを送信
    }

    // ディスクリプタヒープを設定し、シャドウマップをサンプリングできるようにする
    ID3D12DescriptorHeap* heaps[] = { m_pDescriptorHeap->GetHeap() };
    m_pCommandList->SetDescriptorHeaps(1, heaps);

    // カメラ視点からシーンを描画 (シャドウを計算し、カラー出力)
    for (size_t i = 0; i < m_meshes.size(); i++)
    {
        m_pCommandList->SetGraphicsRootDescriptorTable(2, m_pDescriptorHeap->GetGpuDescriptorHandle((int)i));

        Mesh* pMesh = m_meshes[i];

        if (pMesh->contentsBuffer) {
            m_pCommandList->SetGraphicsRootConstantBufferView(4, pMesh->contentsBuffer->GetGPUVirtualAddress());      //頂点数を送信
        }

        if (pMesh->shapeDeltasBuffer) {
            m_pCommandList->SetGraphicsRootShaderResourceView(5, pMesh->shapeDeltasBuffer->GetGPUVirtualAddress());   //シェイプキーの位置情報を送信
        }

        if (pMesh->shapeWeightsBuffer) {
            m_pCommandList->SetGraphicsRootShaderResourceView(6, pMesh->shapeWeightsBuffer->GetGPUVirtualAddress());  //シェイプキーのウェイト情報を送信
        }

        m_meshes[i]->Draw(m_pCommandList); // カラーとシャドウを計算し描画
    }
}

void Model::Update(UINT backBufferIndex)
{
    //位置、回転、スケールを決定
    ModelConstantBuffer mcb{};
    XMMATRIX scale = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
    XMMATRIX rotX = XMMatrixRotationX(XMConvertToRadians(m_rotation.x));
    XMMATRIX rotY = XMMatrixRotationY(XMConvertToRadians(m_rotation.y));
    XMMATRIX rotZ = XMMatrixRotationZ(XMConvertToRadians(m_rotation.z));
    XMMATRIX rot = rotX * rotY * rotZ;
    XMMATRIX pos = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
    mcb.modelMatrix = scale * rot * pos;
    mcb.viewMatrix = XMMatrixLookAtRH(m_pCamera->m_eyePos, m_pCamera->m_targetPos, m_pCamera->m_upFoward);
    mcb.projectionMatrix = XMMatrixPerspectiveFovRH(m_pCamera->m_fov, m_pCamera->m_aspect, m_pCamera->m_near, m_pCamera->m_far);
    mcb.lightViewProjMatrix = m_pDirectionalLight->lightViewProj;
    mcb.normalMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, mcb.modelMatrix));

    //定数バッファにデータを書き込む
    void* p0;
    HRESULT hr = m_modelConstantBuffer[backBufferIndex]->Map(0, nullptr, &p0);
    if (p0) {
        memcpy(p0, &mcb, sizeof(ModelConstantBuffer));
        m_modelConstantBuffer[backBufferIndex]->Unmap(0, nullptr);
    }
    if (FAILED(hr)) {
        printf("バッファの更新に失敗しました。エラーコード:%d\n", hr);

        HRESULT reason = m_pDevice->GetDeviceRemovedReason();
        if (reason != S_OK) {
            // デバイス削除の原因をログに出力
            printf("Device removed reason: 0x%08X\n", reason);
        }
    }

    void* p1;
    m_lightConstantBuffer->Map(0, nullptr, &p1);
    if (p1) {
        memcpy(p1, &m_pDirectionalLight->lightBuffer, sizeof(LightBuffer));
        m_lightConstantBuffer->Unmap(0, nullptr);
    }

    XMVECTOR objPos = XMLoadFloat3(&m_position);
    XMVECTOR camPos = m_pCamera->m_eyePos;
    m_depth = XMVectorGetZ(XMVector3Length(objPos - camPos)); //Z成分を深度として取得

}

void Model::CreateBuffer(Mesh* pMesh, std::vector<VertexPrimitive>& vertices, std::vector<UINT>& indices)
{
    //頂点バッファを設定
    const UINT vertexBufferSize = static_cast<UINT>(sizeof(VertexPrimitive) * vertices.size());

    //ヒープ設定
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    //頂点バッファのリソース
    D3D12_RESOURCE_DESC vertexBufferDesc = {};
    vertexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vertexBufferDesc.Width = vertexBufferSize;
    vertexBufferDesc.Height = 1;
    vertexBufferDesc.DepthOrArraySize = 1;
    vertexBufferDesc.MipLevels = 1;
    vertexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    vertexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    vertexBufferDesc.SampleDesc.Count = 1;

    //デバイスで作成
    HRESULT result = m_pDevice->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&pMesh->vertexBuffer)
    );

    if (FAILED(result)) {
        printf("頂点バッファの生成に失敗しました。\n");
    }


    //頂点データをGPUに送信
    void* vertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    pMesh->vertexBuffer->Map(0, &readRange, &vertexDataBegin);
    memcpy(vertexDataBegin, vertices.data(), vertexBufferSize);
    pMesh->vertexBuffer->Unmap(0, nullptr);

    pMesh->vertexBufferView.BufferLocation = pMesh->vertexBuffer->GetGPUVirtualAddress();
    pMesh->vertexBufferView.StrideInBytes = sizeof(VertexPrimitive);
    pMesh->vertexBufferView.SizeInBytes = vertexBufferSize;

    //インデックスバッファの作成
    const UINT indexBufferSize = static_cast<UINT>(sizeof(UINT) * indices.size());

    //インデックスバッファのリソース
    D3D12_RESOURCE_DESC indexBufferDesc = {};
    indexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    indexBufferDesc.Width = indexBufferSize;
    indexBufferDesc.Height = 1;
    indexBufferDesc.DepthOrArraySize = 1;
    indexBufferDesc.MipLevels = 1;
    indexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    indexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    indexBufferDesc.SampleDesc.Count = 1;

    result = m_pDevice->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &indexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&pMesh->indexBuffer)
    );

    if (FAILED(result)) {
        printf("インデックスバッファの生成に失敗しました。\n");
    }

    //インデックスデータをGPUに送信
    void* indexDataBegin;
    pMesh->indexBuffer->Map(0, &readRange, &indexDataBegin);
    memcpy(indexDataBegin, indices.data(), indexBufferSize);
    pMesh->indexBuffer->Unmap(0, nullptr);

    pMesh->indexBufferView.BufferLocation = pMesh->indexBuffer->GetGPUVirtualAddress();
    pMesh->indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    pMesh->indexBufferView.SizeInBytes = indexBufferSize;

    pMesh->indexCount = static_cast<UINT>(indices.size());
}

Model::Mesh::Mesh()
    : vertexBuffer(nullptr)
    , indexBuffer(nullptr)
    , contentsBuffer(nullptr)
    , shapeWeightsBuffer(nullptr)
    , shapeDeltasBuffer(nullptr)
    , vertexBufferView(D3D12_VERTEX_BUFFER_VIEW())
    , indexBufferView(D3D12_INDEX_BUFFER_VIEW())
    , indexCount(0)
{
}

void Model::Mesh::Draw(ID3D12GraphicsCommandList* pCommandList) const
{
    pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);           //頂点情報を送信
    pCommandList->IASetIndexBuffer(&indexBufferView);                    //インデックス情報を送信
    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);  //3角ポリゴンのみ

    pCommandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);  //描画
}
