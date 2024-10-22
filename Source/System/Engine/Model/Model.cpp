#include "Model.h"
#include <d3dx12.h>
#include <stdexcept>
#include "..\\..\\Main\\Main.h"

ID3D12Resource* modelConstantBuffer;

Model::Model(const Camera* pCamera)
    : m_modelMatrix(XMMatrixIdentity())
    , m_pCamera(pCamera)
    , m_position(XMFLOAT3(0.0f, 0.0f, 0.0f))
    , m_rotation(XMFLOAT3(0.0f, 0.0f, 0.0f))
    , m_scale(XMFLOAT3(1.0f, 1.0f, 1.0f))
    , m_pDescriptorHeap(nullptr)
    , m_pPipelineState(nullptr)
    , m_pRootSignature(nullptr)
{
    m_pDevice = g_Engine->Device();

    auto eyePos = XMVectorSet(0.0f, 5.0f, 1.0f, 0.0f); // 視点の位置
    auto targetPos = XMVectorSet(0.0f, 0.0f, 0.75f, 0.0f); // 視点を向ける座標
    auto upward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // 上方向を表すベクトル
    constexpr auto fov = XMConvertToRadians(37.5); // 視野角
    auto aspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT); // アスペクト比

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    CD3DX12_RESOURCE_DESC constantData = CD3DX12_RESOURCE_DESC::Buffer(sizeof(ModelConstantBuffer));
    //定数バッファをリソースとして作成
    for (int i = 0; i < FRAME_BUFFER_COUNT; i++) {
        m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &constantData,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_modelConstantBuffer[i]));
    }
}

void Model::LoadModel(const PrimitiveModel primitiveModel)
{
    if (primitiveModel == PrimitiveModel::Primitive_Sphere) {
        LoadSphere(0.2f, 32, 32, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    m_pRootSignature = new RootSignature(m_pDevice, ShaderKinds::PrimitiveShader);
    m_pPipelineState = new PipelineState(m_pDevice, m_pRootSignature);

    printf("メッシュを生成しました。\n");
}

void Model::LoadModel(const std::string fbxFile)
{
    Assimp::Importer importer;

    //モデル読み込み時のフラグ。メッシュのポリゴンはすべて三角形にし、左手座標系に変換
    unsigned int flag = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
        aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights;// | aiProcess_MakeLeftHanded;
    const aiScene* scene = importer.ReadFile(fbxFile, flag);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        printf("FBXファイルをロードできませんでした。\n");
        return;
    }

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

        vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
        vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
        vertex.Color = { 1.0f, 1.0f,1.0f,1.0f };
        if (mesh->mTextureCoords[0]) {
            vertex.TexCoords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        }
        else {
            vertex.TexCoords = { 0.0f, 0.0f };
        }

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

    meshData->materialIndex = -1;

    return meshData;
}

void Model::Update()
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
    mcb.projectionMatrix = XMMatrixPerspectiveFovRH(m_pCamera->m_fov, m_pCamera->m_aspect, 0.01f, 1000.0f);
    mcb.lightViewProjMatrix = g_Engine->GetDirectionalLight()->lightViewProj;
    mcb.lightDirection = g_Engine->GetDirectionalLight()->lightDirection;

    //定数バッファにデータを書き込む
    void* p;
    UINT bufferIndex = g_Engine->CurrentBackBufferIndex();
    m_modelConstantBuffer[bufferIndex]->Map(0, nullptr, &p);
    memcpy(p, &mcb, sizeof(ModelConstantBuffer));
    m_modelConstantBuffer[bufferIndex]->Unmap(0, nullptr);
}

void Model::Draw()
{
    //エンジンからコマンドリストを取得
    ID3D12GraphicsCommandList* pCommandList = g_Engine->CommandList();

    //現在のバックバッファのインデックス (トリプルバッファリングのため、0～2が返される。フレームの描画が終われば次のインデックスに移行)
    UINT bufferIndex = g_Engine->CurrentBackBufferIndex();

    //コマンドリストに送信
    pCommandList->SetGraphicsRootSignature(m_pRootSignature->GetRootSignature());                                   //ルートシグネチャを設定
    pCommandList->SetPipelineState(m_pPipelineState->GetPipelineState());                                           //パイプラインステートを設定

    pCommandList->SetGraphicsRootConstantBufferView(0, m_modelConstantBuffer[bufferIndex]->GetGPUVirtualAddress()); //モデルの位置関係を送信

    if (m_boneMatricesBuffer) {
        pCommandList->SetGraphicsRootConstantBufferView(1, m_boneMatricesBuffer->GetGPUVirtualAddress());           //ボーンを送信
    }

    if (m_pDescriptorHeap) {
        ID3D12DescriptorHeap* materialHeap = m_pDescriptorHeap->GetHeap();  //ディスクリプタヒープを取得
        pCommandList->SetDescriptorHeaps(1, &materialHeap);                 //ディスクリプタヒープを送信
    }


    //メッシュの数だけ繰り返す
    for (size_t i = 0; i < m_meshes.size(); i++) {
        Mesh* pMesh = m_meshes[i];

        if (pMesh->contentsBuffer) {
            pCommandList->SetGraphicsRootConstantBufferView(2, pMesh->contentsBuffer->GetGPUVirtualAddress());      //頂点数を送信
        }

        if (pMesh->shapeDeltasBuffer) {
            pCommandList->SetGraphicsRootShaderResourceView(3, pMesh->shapeDeltasBuffer->GetGPUVirtualAddress());   //シェイプキーの位置情報を送信
        }

        if (pMesh->shapeWeightsBuffer) {
            pCommandList->SetGraphicsRootShaderResourceView(4, pMesh->shapeWeightsBuffer->GetGPUVirtualAddress());  //シェイプキーのウェイト情報を送信
        }

        pCommandList->IASetVertexBuffers(0, 1, &pMesh->vertexBufferView);           //頂点情報を送信
        pCommandList->IASetIndexBuffer(&pMesh->indexBufferView);                    //インデックス情報を送信
        pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);  //3角ポリゴンのみ

        if (pMesh->materialIndex != -1)
            pCommandList->SetGraphicsRootDescriptorTable(5, g_materials[pMesh->materialIndex]->HandleGPU); //マテリアルを送信

        pCommandList->DrawIndexedInstanced(pMesh->indexCount, 1, 0, 0, 0);          //描画
    }
}

void Model::LoadSphere(float radius, UINT sliceCount, UINT stackCount, const XMFLOAT4 color)
{
    Mesh* meshData = new Mesh();

    std::vector<VertexPrimitive> vertices;
    std::vector<UINT> indices;

    // 頂点の計算
    VertexPrimitive topVertex = { XMFLOAT3(0.0f, +radius, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f), color };
    VertexPrimitive bottomVertex = { XMFLOAT3(0.0f, -radius, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f), color };

    vertices.push_back(topVertex);

    float phiStep = XM_PI / stackCount;
    float thetaStep = 2.0f * XM_PI / sliceCount;

    for (UINT i = 1; i < stackCount; ++i) {
        float phi = i * phiStep;

        for (UINT j = 0; j <= sliceCount; ++j) {
            float theta = j * thetaStep;

            VertexPrimitive v{};

            v.Position.x = radius * sinf(phi) * cosf(theta);
            v.Position.y = radius * cosf(phi);
            v.Position.z = radius * sinf(phi) * sinf(theta);

            v.Color = color;

            vertices.push_back(v);
        }
    }

    vertices.push_back(bottomVertex);

    // インデックスの計算
    for (uint32_t i = 1; i <= sliceCount; ++i) {
        indices.push_back(0);
        indices.push_back(i + 1);
        indices.push_back(i);
    }

    uint32_t baseIndex = 1;
    uint32_t ringVertexCount = sliceCount + 1;
    for (uint32_t i = 0; i < stackCount - 2; ++i) {
        for (uint32_t j = 0; j < sliceCount; ++j) {
            indices.push_back(baseIndex + i * ringVertexCount + j);
            indices.push_back(baseIndex + i * ringVertexCount + j + 1);
            indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);

            indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
            indices.push_back(baseIndex + i * ringVertexCount + j + 1);
            indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
        }
    }

    for (uint32_t i = 0; i < sliceCount; ++i) {
        indices.push_back((uint32_t)vertices.size() - 1);
        indices.push_back(baseIndex + (stackCount - 2) * ringVertexCount + i);
        indices.push_back(baseIndex + (stackCount - 2) * ringVertexCount + i + 1);
    }

    CreateBuffer(meshData, vertices, indices);
    meshData->materialIndex = -1;
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
    , materialIndex(-1)
{
}
