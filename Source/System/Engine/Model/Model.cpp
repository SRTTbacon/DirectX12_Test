#include "Model.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <d3dx12.h>
#include <stdexcept>
#include <unordered_map>
#include "..\\..\\Main\\Main.h"

ID3D12Resource* modelConstantBuffer;

Model::Model(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::string& fbxFile, const Camera* pCamera)
    : device(device)
    , m_modelMatrix(XMMatrixIdentity())
    , m_pCamera(pCamera)
    , m_position(XMFLOAT3(0.0f, 0.0f, 0.0f))
    , m_rotation(XMFLOAT3(90.0f, 0.0f, 0.0f))
    , m_scale(XMFLOAT3(1.0f, 1.0f, 1.0f))
{
    LoadFBX(fbxFile);

    m_pRootSignature = new RootSignature(device);
    m_pPipelineState = new PipelineState(device, m_pRootSignature->GetRootSignature());

    auto eyePos = XMVectorSet(0.0f, 5.0f, 1.0f, 0.0f); // 視点の位置
    auto targetPos = XMVectorSet(0.0f, 0.0f, 0.75f, 0.0f); // 視点を向ける座標
    auto upward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // 上方向を表すベクトル
    constexpr auto fov = XMConvertToRadians(37.5); // 視野角
    auto aspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT); // アスペクト比

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    CD3DX12_RESOURCE_DESC d = CD3DX12_RESOURCE_DESC::Buffer(sizeof(ModelConstantBuffer));
    // 定数バッファをリソースとして作成
    for (int i = 0; i < FRAME_BUFFER_COUNT; i++) {
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &d,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&modelConstantBuffer[i]));
    }

    printf("モデル:%sをロードしました。\n", fbxFile.c_str());
}

void Model::LoadFBX(const std::string& fbxFile) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(fbxFile, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType | aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights | aiProcess_MakeLeftHanded);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        printf("FBXファイルをロードできませんでした。\n");
        return;
    }

    ProcessNode(scene, scene->mRootNode);

    //ボーンに親子関係を付ける
    LoadBoneFamily(scene->mRootNode);


    XMFLOAT3 rot = XMFLOAT3(0.0f, 0.0f, 0.0f);
    UpdateBoneRotation("Head", rot);

    //マテリアルの読み込み
    descriptorHeap = new DescriptorHeap();

    std::string dir = "Resource\\Model\\";
    for (size_t i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[i];
        std::string nameOnly = "";
        // メッシュのマテリアルを取得する
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            //ファイル名 = マテリアル名 + .png
            nameOnly = material->GetName().C_Str() + std::string(".png");
            std::string texPath = dir + nameOnly;
            
            //テクスチャがまだロードされていない場合はロードし、ロード済みの場合は入っているインデックスを参照
            int index = Texture2D::GetTextureIndex(texPath);
            if (index == -1) {
                //テクスチャを作成
                Texture2D* mainTex = Texture2D::Get(texPath);
                //マテリアルを作成
                DescriptorHandle* handle = descriptorHeap->Register(mainTex);

                g_materials.push_back(handle);
                meshes[i].materialIndex = (BYTE)(g_materials.size() - 1);
            }
            else {
                meshes[i].materialIndex = (BYTE)index;
                g_materials[index]->UseCount++;
            }
        }
    }
}

void Model::ProcessNode(const aiScene* scene, aiNode* node) {
    // メッシュを処理
    for (UINT i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(ProcessMesh(scene, mesh));
    }

    // 子ノードも再帰的に処理
    for (UINT i = 0; i < node->mNumChildren; i++) {
        ProcessNode(scene, node->mChildren[i]);
    }
}

Model::Mesh Model::ProcessMesh(const aiScene* scene, aiMesh* mesh) {
    std::vector<Vertex> vertices;
    std::vector<UINT> indices;

    // 頂点の処理
    for (UINT i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex{};

        vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
        vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
        vertex.Color = { 1.0f, 1.0f,1.0f,1.0f };
        if (mesh->mTextureCoords[0]) {
            vertex.TexCoords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        }
        else {
            vertex.TexCoords = { 0.0f, 0.0f };
        }

        vertex.BoneWeights = { 0.0f, 0.0f, 0.0f, 0.0f };
        vertex.BoneIDs[0] = vertex.BoneIDs[1] = vertex.BoneIDs[2] = vertex.BoneIDs[3] = 0;

        vertices.push_back(vertex);
    }

    // インデックスの処理
    for (UINT i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (UINT j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // 頂点バッファの作成
    Mesh meshData;

    // ボーンの処理
    LoadBones(scene, meshData, mesh, vertices);

    //頂点バッファを設定
    const UINT vertexBufferSize = static_cast<UINT>(sizeof(Vertex) * vertices.size());

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
    HRESULT result = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&meshData.vertexBuffer)
    );

    if (FAILED(result)) {
        printf("頂点バッファの生成に失敗しました。\n");
    }

    //頂点データをGPUに送信
    void* vertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    meshData.vertexBuffer->Map(0, &readRange, &vertexDataBegin);
    memcpy(vertexDataBegin, vertices.data(), vertexBufferSize);
    meshData.vertexBuffer->Unmap(0, nullptr);

    meshData.vertexBufferView.BufferLocation = meshData.vertexBuffer->GetGPUVirtualAddress();
    meshData.vertexBufferView.StrideInBytes = sizeof(Vertex);
    meshData.vertexBufferView.SizeInBytes = vertexBufferSize;

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

    result = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &indexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&meshData.indexBuffer)
    );

    if (FAILED(result)) {
        printf("インデックスバッファの生成に失敗しました。\n");
    }

    //インデックスデータをGPUに送信
    void* indexDataBegin;
    meshData.indexBuffer->Map(0, &readRange, &indexDataBegin);
    memcpy(indexDataBegin, indices.data(), indexBufferSize);
    meshData.indexBuffer->Unmap(0, nullptr);

    meshData.indexBufferView.BufferLocation = meshData.indexBuffer->GetGPUVirtualAddress();
    meshData.indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    meshData.indexBufferView.SizeInBytes = indexBufferSize;

    meshData.indexCount = static_cast<UINT>(indices.size());

    //ボーン情報のリソースを作成
    CD3DX12_RESOURCE_DESC d = CD3DX12_RESOURCE_DESC::Buffer(sizeof(XMMATRIX) * boneInfos.size());

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &d,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_boneMatricesBuffer));
    if (FAILED(hr)) {
        printf("ボーンバッファの生成に失敗しました。%1x\n", hr);
    }

    return meshData;
}

void Model::LoadBones(const aiScene* scene, Mesh& meshStruct, aiMesh* mesh, std::vector<Vertex>& vertices)
{
    for (UINT i = 0; i < mesh->mNumBones; i++) {
        aiBone* bone = mesh->mBones[i];
        UINT boneIndex = 0;

        // ボーンがまだ登録されていない場合、マッピングを追加
        if (boneMapping.find(bone->mName.C_Str()) == boneMapping.end()) {
            boneIndex = static_cast<UINT>(boneInfos.size());

            //配列に追加
            boneInfos.push_back(XMMatrixIdentity());
            finalBoneInfos.push_back(XMMatrixIdentity());
            boneWorlds.push_back(BoneNode{});

            boneWorlds[boneIndex].boneName = bone->mName.C_Str();
            //boneInfos[boneIndex] = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<XMFLOAT4X4*>(&bone->mOffsetMatrix)));
            boneInfos[boneIndex] = XMMatrixIdentity();

            //ボーンの位置、回転、スケールを初期化
            boneWorlds[boneIndex].m_position = XMFLOAT3(0.0f, 0.0f, 0.0f);
            boneWorlds[boneIndex].m_rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
            boneWorlds[boneIndex].m_scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
            boneWorlds[boneIndex].boneOffset = XMLoadFloat4x4(reinterpret_cast<XMFLOAT4X4*>(&bone->mOffsetMatrix));

            //ボーン名とインデックスを紐づけ
            boneMapping[bone->mName.C_Str()] = boneIndex;
        }
        else {
            boneIndex = boneMapping[bone->mName.C_Str()];
        }

        for (UINT j = 0; j < bone->mNumWeights; j++) {
            UINT vertexID = bone->mWeights[j].mVertexId;
            float weight = bone->mWeights[j].mWeight;

            Vertex& vertex = vertices[vertexID];

            // 空いているボーンウェイトスロットを探す
            if (vertex.BoneWeights.x == 0.0f) {
                vertex.BoneIDs[0] = boneIndex;
                vertex.BoneWeights.x = weight;
            }
            else if (vertex.BoneWeights.y == 0.0f) {
                vertex.BoneIDs[1] = boneIndex;
                vertex.BoneWeights.y = weight;
            }
            else if (vertex.BoneWeights.z == 0.0f) {
                vertex.BoneIDs[2] = boneIndex;
                vertex.BoneWeights.z = weight;
            }
            else if (vertex.BoneWeights.w == 0.0f) {
                vertex.BoneIDs[3] = boneIndex;
                vertex.BoneWeights.w = weight;
            }
            else {
                // 4つ以上のボーンウェイトはサポート外
                //printf("頂点 %d は 4 つ以上のボーンウェイトを持っています。\n", vertexID);
            }
        }
    }

    for (size_t i = 0; i < vertices.size(); i++) {
        Vertex& v = vertices[i];
        //printf("vertices[%1x] = %f, %f, %f, %f\n", i, v.BoneWeights.x, v.BoneWeights.y, v.BoneWeights.z, v.BoneWeights.w);
    }
}

//ボーンの親子関係を取得
void Model::LoadBoneFamily(const aiNode* node)
{
    if (boneMapping.find(node->mName.C_Str()) != boneMapping.end()) {
        UINT boneIndex = boneMapping[node->mName.C_Str()];
        for (UINT i = 0; i < node->mNumChildren; i++) {
            if (boneMapping.find(node->mChildren[i]->mName.C_Str()) != boneMapping.end()) {
                UINT childIndex = boneMapping[node->mChildren[i]->mName.C_Str()];
                boneWorlds[boneIndex].childBones.push_back(childIndex);
                boneWorlds[childIndex].parentBoneIndex = boneIndex;
            }
        }
    }
    for (UINT i = 0; i < node->mNumChildren; i++) {
        LoadBoneFamily(node->mChildren[i]);
    }
}

void Model::UpdateBonePosition(std::string boneName, XMFLOAT3& position)
{
    UINT boneIndex = boneMapping[boneName];
    if (boneIndex == 0)
        return;

    boneWorlds[boneIndex].m_position = position;
}
void Model::UpdateBoneRotation(std::string boneName, XMFLOAT3& rotation)
{
    UINT boneIndex = boneMapping[boneName];
    if (boneIndex == 0)
        return;

    boneWorlds[boneIndex].m_rotation = rotation;
}
void Model::UpdateBoneScale(std::string boneName, XMFLOAT3& scale)
{
    UINT boneIndex = boneMapping[boneName];
    if (boneIndex == 0)
        return;

    boneWorlds[boneIndex].m_scale = scale;
}

void Model::UpdateBoneTransform(UINT boneIndex, XMMATRIX& parentMatrix)
{
    /*for (UINT i = 0; i < addedBoneIndexies.size(); i++) {
        if (addedBoneIndexies[i] == boneIndex) {
            //printf("既に実行済みのboneIndex:%dです。\n", boneIndex);
            return;
        } 
    }*/
    /*
    XMMATRIX childMatrix = parentMatrix;
    //addedBoneIndexies.push_back(boneIndex);
    boneInfos[boneIndex] = parentMatrix * boneWorlds[boneIndex].boneTransform;
    childMatrix = boneInfos[boneIndex];
    for (UINT nextBoneIndex : boneWorlds[boneIndex].childBones) {
        //printf("NextBoneIndex = %d\n", nextBoneIndex);
        UpdateBoneTransform(nextBoneIndex, childMatrix);
    }*/

    XMMATRIX parentTransform = XMMatrixIdentity();

    // 親ボーンが存在する場合、そのワールド変換を適用
    if (boneWorlds[boneIndex].parentBoneIndex != -1) {
        parentTransform = boneInfos[boneWorlds[boneIndex].parentBoneIndex];
    }

    BoneNode& node = boneWorlds[boneIndex];
    XMMATRIX scale = XMMatrixScaling(node.m_scale.x, node.m_scale.y, node.m_scale.z);
    XMMATRIX rotX = XMMatrixRotationX(XMConvertToRadians(node.m_rotation.x));
    XMMATRIX rotY = XMMatrixRotationY(XMConvertToRadians(node.m_rotation.y));
    XMMATRIX rotZ = XMMatrixRotationZ(XMConvertToRadians(node.m_rotation.z));
    XMMATRIX rot = rotX * rotY * rotZ;
    XMMATRIX pos = XMMatrixTranslation(node.m_position.x, node.m_position.y, node.m_position.z);
    //XMMATRIX aaa = XMMatrixTranslation(node.boneOffset.r[3].m128_f32[0], node.boneOffset.r[3].m128_f32[1], node.boneOffset.r[3].m128_f32[2]);
    //XMMATRIX bbb = XMMatrixTranslation(-node.boneOffset.r[3].m128_f32[0], -node.boneOffset.r[3].m128_f32[1], -node.boneOffset.r[3].m128_f32[2]);
    XMMATRIX boneTransform = scale * rot * pos;

    XMMATRIX finalTransform = XMMatrixTranspose(boneTransform) * parentTransform;

    // 親のワールド変換とローカル変換を合成
    boneInfos[boneIndex] = finalTransform;
    //finalBoneInfos[boneIndex] = boneInfos[boneIndex] * node.boneOffset;

    if (node.boneName == "Head") {
        XMFLOAT4 a = XMFLOAT4(boneInfos[boneIndex].r[0].m128_f32[0], boneInfos[boneIndex].r[0].m128_f32[1], boneInfos[boneIndex].r[0].m128_f32[2], boneInfos[boneIndex].r[0].m128_f32[3]);
        XMFLOAT4 b = XMFLOAT4(boneInfos[boneIndex].r[1].m128_f32[0], boneInfos[boneIndex].r[1].m128_f32[1], boneInfos[boneIndex].r[1].m128_f32[2], boneInfos[boneIndex].r[1].m128_f32[3]);
        XMFLOAT4 c = XMFLOAT4(boneInfos[boneIndex].r[2].m128_f32[0], boneInfos[boneIndex].r[2].m128_f32[1], boneInfos[boneIndex].r[2].m128_f32[2], boneInfos[boneIndex].r[2].m128_f32[3]);
        XMFLOAT4 d = XMFLOAT4(boneInfos[boneIndex].r[3].m128_f32[0], boneInfos[boneIndex].r[3].m128_f32[1], boneInfos[boneIndex].r[3].m128_f32[2], boneInfos[boneIndex].r[3].m128_f32[3]);
        printf("a1:%f, a2:%f, a3:%f, a4:%f\nb1:%f, b2:%f, b3:%f, b4:%f\nc1:%f, c2:%f, c3:%f, c4:%f\nd1:%f, d2:%f, d3:%f, d4:%f\n", a.x, a.y, a.z, a.w,
            b.x, b.y, b.z, b.w, c.x, c.y, c.z, c.w, d.x, d.y, d.z, d.w);
        // 行列の4行目（平行移動ベクトル）を抽出してワールド座標を取得
        XMFLOAT3 worldPosition{};
        worldPosition.x = boneInfos[boneIndex].r[3].m128_f32[0];
        worldPosition.y = boneInfos[boneIndex].r[3].m128_f32[1];
        worldPosition.z = boneInfos[boneIndex].r[3].m128_f32[2];
        printf("PosX = %f, PosY = %f, PosZ = %f\n", worldPosition.x, worldPosition.y, worldPosition.z);
    }

    // 子ボーンにも変換を伝播
    for (UINT childBoneIndex : boneWorlds[boneIndex].childBones) {
        UpdateBoneTransform(childBoneIndex, boneInfos[boneIndex]);
    }
}

void Model::UpdateBoneTransform()
{
    XMMATRIX m = XMMatrixIdentity();
    UpdateBoneTransform(0, m);

    void* pData;
    HRESULT hr = m_boneMatricesBuffer->Map(0, nullptr, &pData);
    if (SUCCEEDED(hr))
    {
        memcpy(pData, boneInfos.data(), sizeof(XMMATRIX) * boneInfos.size()); // ボーンマトリックスをコピー
        m_boneMatricesBuffer->Unmap(0, nullptr);
    }
}

void Model::Update()
{
    UpdateBoneTransform();

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

    //定数バッファにデータを書き込む
    void* p;
    UINT bufferIndex = g_Engine->CurrentBackBufferIndex();
    modelConstantBuffer[bufferIndex]->Map(0, nullptr, &p);
    memcpy(p, &mcb, sizeof(ModelConstantBuffer));
    modelConstantBuffer[bufferIndex]->Unmap(0, nullptr);
}

void Model::Draw(ID3D12GraphicsCommandList* commandList)
{
    UINT bufferIndex = g_Engine->CurrentBackBufferIndex();

    commandList->SetGraphicsRootSignature(m_pRootSignature->GetRootSignature());
    commandList->SetPipelineState(m_pPipelineState->GetPipelineState());
    commandList->SetGraphicsRootConstantBufferView(0, modelConstantBuffer[bufferIndex]->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, m_boneMatricesBuffer->GetGPUVirtualAddress());

    ID3D12DescriptorHeap* materialHeap = descriptorHeap->GetHeap();

    for (size_t i = 0; i < meshes.size(); i++) {
        Mesh& mesh = meshes[i];

        commandList->IASetVertexBuffers(0, 1, &mesh.vertexBufferView);
        commandList->IASetIndexBuffer(&mesh.indexBufferView);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        commandList->SetDescriptorHeaps(1, &materialHeap);
        commandList->SetGraphicsRootDescriptorTable(2, g_materials[mesh.materialIndex]->HandleGPU);

        commandList->DrawIndexedInstanced(mesh.indexCount, 1, 0, 0, 0);
    }
}
