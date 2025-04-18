#include "Character.h"

std::unordered_map<std::string, Character::HumanoidList> Character::s_sharedHumanoidMeshes;

Character::Character(const std::string modelFile, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const Camera* pCamera, const DirectionalLight* pDirectionalLight, 
    MaterialManager* pMaterialManager, float* pFrameTime)
    : Model(pDevice, pCommandList, pCamera, pDirectionalLight, pMaterialManager, pFrameTime)
    , m_boneManager(BoneManager(&m_position, &m_rotation, &m_scale))
    , m_bHCSFile(false)
{
    DWORD startTime = timeGetTime();
    m_modelFile = modelFile;
    m_modelType = ModelType_Character;

    CreateConstantBuffer();

    DWORD tempTime = timeGetTime() - startTime;
    //printf("CreateConstantBuffer - %dms\n", tempTime);

    BinaryReader br(modelFile);
    char* headerBuf = br.ReadBytes(br.ReadByte());
    std::string header = headerBuf;
    delete[] headerBuf;
    br.Close();

    //ヘッダーがhcsファイルだったら
    if (header == std::string(MODEL_HEADER)) {
        LoadHCS(modelFile);
    }
    else {
        LoadFBX(modelFile);
    }

    tempTime = timeGetTime();

    m_boneManager.UpdateBoneMatrix();

    DWORD loadTime = timeGetTime() - startTime;
	printf("モデル:%sをロードしました。 %dms\n", modelFile.c_str(), loadTime);

    //g_Engine->EndRender();
    //g_Engine->BeginRender();
}

Character::~Character()
{
    for (HumanoidMesh& humanoidMesh : m_humanoidMeshes) {
        if (humanoidMesh.pMesh->shapeWeightsBuffer) {
            humanoidMesh.pMesh->shapeWeightsBuffer->Unmap(0, nullptr);
            humanoidMesh.pMesh->pShapeWeightsMapped = nullptr;
        }
    }
    if (m_boneMatricesBuffer) {
        m_boneMatricesBuffer->Unmap(0, nullptr);
        m_pBoneMatricesMap = nullptr;
    }

    Model::~Model();
}

UINT Character::AddAnimation(Animation* pAnimation)
{
    CharacterAnimation anim{ pAnimation, 0 };
    m_characterAnimations.push_back(anim);
	//アニメーションが1つしかない場合は、現在のアニメーションインデックスを0にする
    if (m_characterAnimations.size() == 1) {
        m_nowAnimationIndex = 0;
    }
    return (unsigned int)(m_characterAnimations.size() - 1);
}

void Character::Update()
{
    //アニメーションの更新
    UpdateAnimation();

    //ボーンのマトリックスを更新
    m_boneManager.UpdateBoneMatrix();

    //シェイプキーのウェイトを更新
    UpdateShapeKeys();
}

void Character::LateUpdate(UINT backBufferIndex)
{
    //ボーンバッファに送信
    memcpy(m_pBoneMatricesMap, m_boneManager.m_boneInfos.data(), sizeof(XMMATRIX) * m_boneManager.m_boneInfos.size()); //ボーンマトリックスをコピー

    Model::LateUpdate(backBufferIndex);

    //キャラクターはルートモーションを考慮
    XMFLOAT3 armaturePos = m_boneManager.m_armatureBone.m_position;
    armaturePos.x = armaturePos.x;
    armaturePos.z = -armaturePos.z;
    XMFLOAT3 hipPos = XMFLOAT3(0.0f, 0.0f, 0.0f);
    if (m_boneManager.m_bones.size() > 0) {
        //m_bonesの0番目は基本的にルートボーン
        hipPos = m_boneManager.m_bones[0].m_position;
        float temp = hipPos.x;
        hipPos.x = -hipPos.z;
        hipPos.z = -temp;
    }
    //カメラ位置からオブジェクト位置までの距離をm_depthに設定
    XMFLOAT3 tempPos = armaturePos + hipPos + m_position;
    XMFLOAT3 camPos;
    XMStoreFloat3(&camPos, m_pCamera->m_eyePos);
    m_depth = DistanceSq(tempPos, camPos);
}

void Character::LoadFBX(const std::string& fbxFile)
{
    Assimp::Importer importer;

    //モデル読み込み時のフラグ。メッシュのポリゴンはすべて三角形にし、ボーンが存在する場合は影響を受けるウェイトを4つまでにする
    UINT flag = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
        aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights | aiProcess_OptimizeMeshes;
    const aiScene* scene = importer.ReadFile(fbxFile, flag);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        printf("FBXファイルをロードできませんでした。\n");
        return;
    }

    ProcessNode(scene, scene->mRootNode);

    //ボーンに親子関係を付ける
    LoadBoneFamily(scene->mRootNode);

    XMMATRIX mat = XMMatrixIdentity();
    CalculateBoneTransforms(scene->mRootNode, mat);

    for (HumanoidMesh& humanoidMesh : m_humanoidMeshes) {
        CreateHumanoidMeshBuffer(humanoidMesh);
    }

    //シェイプキーの位置情報を保持するリソースを作成
    CreateShapeDeltasTexture(false);

    //マテリアルの読み込み
    for (size_t i = 0; i < scene->mNumMeshes; i++)
    {
        m_meshes[i]->pModel = this;

        aiMesh* mesh = scene->mMeshes[i];
        //メッシュのマテリアルを取得する
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            //ファイル名 = マテリアル名 + .png
            std::string nameOnly = material->GetName().C_Str() + std::string(".png");

            SetTexture(m_meshes[i], nameOnly);
        }
        else {
            //テクスチャが存在しない場合は白単色テクスチャを使用
            m_meshes[i]->pMaterial = m_pMaterialManager->AddMaterialWithShapeData("BoneWhite", m_meshes[i]->meshData->shapeDataIndex);
        }
    }
}

void Character::LoadHCS(const std::string& hcsFile)
{
    DWORD startTime = timeGetTime();
    //ファイルが存在しない
    if (!std::filesystem::exists(hcsFile)) {
        return;
    }

    //バイナリファイルとして開く
    BinaryReader br(hcsFile);

    //ヘッダー
    char* headerBuf = br.ReadBytes(br.ReadByte());
	std::string header = headerBuf;
    delete[] headerBuf;

    //ヘッダーがモデル形式でない場合
    if (header != std::string(MODEL_HEADER)) {
        br.Close();
        return;
    }

    //モデルのフォーマット
    BYTE format = br.ReadByte();
    //キャラクター以外は処理しない
    if (format != MODEL_CHARACTER) {
        br.Close();
        return;
    }

	//ボーン、シェイプキー、メッシュの読み込み
    LoadBones(br);
    DWORD endTime = timeGetTime();
    //printf("LoadBone - %dms\n", endTime - startTime);
    startTime = endTime;
    LoadHumanoidMesh(br);
    endTime = timeGetTime();
    //printf("LoadHumanoidMesh - %dms\n", endTime - startTime);
    startTime = endTime;
    UINT meshCount = br.ReadUInt32();
    for (UINT i = 0; i < meshCount; i++) {
        ProcessMesh(br, m_humanoidMeshes[i], i);
        m_meshes.push_back(m_humanoidMeshes[i].pMesh);
    }

    endTime = timeGetTime();
    //printf("ProcessMesh - %dms\n", endTime - startTime);
    startTime = endTime;

    for (HumanoidMesh& humanoidMesh : m_humanoidMeshes) {
        CreateHumanoidMeshBuffer(humanoidMesh);
    }

    //シェイプキーの位置情報を保持するリソースを作成
    CreateShapeDeltasTexture(true);

    endTime = timeGetTime();
    //printf("CreateHumanoidMeshBuffer - %dms\n", endTime - startTime);
    startTime = endTime;

    //マテリアルの読み込み
    for (UINT i = 0; i < meshCount; i++) {
        bool bExistTexture = br.ReadBoolean();

        m_meshes[i]->pModel = this;

        //メッシュのマテリアルを取得する
        if (bExistTexture) {
            char* nameBuf = br.ReadBytes(br.ReadByte());
            std::string nameOnly = nameBuf;
            delete[] nameBuf;

            SetTexture(m_meshes[i], nameOnly);
        }
        else {
            //テクスチャが存在しない場合は白単色テクスチャを使用
            m_meshes[i]->pMaterial = m_pMaterialManager->AddMaterialWithShapeData("BoneWhite", m_meshes[i]->meshData->shapeDataIndex);
        }
    }

    m_bHCSFile = true;

    endTime = timeGetTime();
    //printf("SetTexture - %dms\n", endTime - startTime);
}

void Character::ProcessNode(const aiScene* pScene, const aiNode* pNode)
{
    //ノードにメッシュが含まれていたら読み込む
    for (UINT j = 0; j < pNode->mNumMeshes; j++) {
        aiMesh* mesh = pScene->mMeshes[pNode->mMeshes[j]];
        HumanoidMesh humanoidMesh{};
        Mesh* pMesh = ProcessMesh(mesh, humanoidMesh, static_cast<UINT>(m_meshes.size()));
        pMesh->meshName = UTF8ToShiftJIS(pNode->mName.C_Str());
        m_humanoidMeshes.push_back(humanoidMesh);
        m_meshes.push_back(pMesh);
    }

    for (UINT i = 0; i < pNode->mNumChildren; i++) {
        aiNode* childNode = pNode->mChildren[i];
        ProcessNode(pScene, childNode);
    }
}

Mesh* Character::ProcessMesh(aiMesh* mesh, HumanoidMesh& humanoidMesh, UINT meshIndex) {
    std::vector<Vertex> vertices;
    std::vector<UINT> indices;

    //頂点の処理
    for (UINT i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex{};

        vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
        vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
        if (mesh->mTextureCoords[0]) {
            vertex.texCoords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        }
        else {
            vertex.texCoords = { 0.0f, 0.0f };
        }
		vertex.tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
		vertex.bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };

        vertex.boneWeights = { 0.0f, 0.0f, 0.0f, 0.0f };
        vertex.boneIDs[0] = vertex.boneIDs[1] = vertex.boneIDs[2] = vertex.boneIDs[3] = 0;

        vertex.vertexID = i;

        vertices.push_back(vertex);
    }

    //インデックスの処理
    for (UINT i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (UINT j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    //頂点数を保存
    humanoidMesh.vertexCount = static_cast<UINT>(vertices.size());

    Mesh* meshData = new Mesh();
    humanoidMesh.pMesh = meshData;

    //ボーンの処理
    LoadBones(mesh, vertices);

    //シェイプキーの処理
    LoadShapeKey(mesh, vertices, humanoidMesh);

    //シェーダーに必要なバッファを生成
    Model::CreateBuffer(meshData, vertices, indices, sizeof(Vertex), meshIndex);
    CreateBuffer(humanoidMesh);

    return meshData;
}

void Character::ProcessMesh(BinaryReader& br, HumanoidMesh& humanoidMesh, UINT meshIndex)
{
    UINT meshBufferOriginalSize = br.ReadUInt32();
    UINT meshBufferCompressedSize = br.ReadUInt32();

    char* compressedBuffer = br.ReadBytes(meshBufferCompressedSize);

    //既に同じモデルがロードされていれば、それを参照
    if (s_sharedMeshes.find(m_modelFile) != s_sharedMeshes.end() && s_sharedMeshes[m_modelFile].meshDataList.size() > meshIndex) {
        std::vector<Vertex> vertex;
        std::vector<UINT> index;
        std::shared_ptr<MeshData> meshData = s_sharedMeshes[m_modelFile].meshDataList[meshIndex];
        Model::CreateBuffer(humanoidMesh.pMesh, vertex, index, sizeof(Vertex), meshIndex);

        //頂点数を保存
        humanoidMesh.vertexCount = meshData->vertexCount;
        CreateBuffer(humanoidMesh);

        delete[] compressedBuffer;

        return;
    }

    //圧縮されているボーン情報を解凍
    std::vector<char> meshBuffer;
    BinaryDecompress(meshBuffer, meshBufferOriginalSize, compressedBuffer, meshBufferCompressedSize);

    delete[] compressedBuffer;

    BinaryReader meshReader(meshBuffer);

    //解凍したバッファを読み込む
    std::vector<Vertex> vertices;
    std::vector<UINT> indices;

    //頂点の処理
    UINT vertexCount = meshReader.ReadUInt32();
    for (UINT i = 0; i < vertexCount; i++) {
        Vertex vertex{};

        vertex.position = meshReader.ReadFloat3();
        vertex.normal = meshReader.ReadFloat3();
        vertex.texCoords = meshReader.ReadFloat2();
        vertex.tangent = meshReader.ReadFloat3();
        vertex.bitangent = meshReader.ReadFloat3();

        vertex.boneWeights = { 0.0f, 0.0f, 0.0f, 0.0f };
        vertex.boneIDs[0] = vertex.boneIDs[1] = vertex.boneIDs[2] = vertex.boneIDs[3] = 0;

        vertex.vertexID = i;

        vertices.push_back(vertex);
    }

    //インデックスの処理
    UINT faceCount = meshReader.ReadUInt32();
    for (UINT i = 0; i < faceCount; i++) {
        UINT indexCount = meshReader.ReadUInt32();
        for (UINT j = 0; j < indexCount; j++) {
            indices.push_back(meshReader.ReadUInt32());
        }
    }

    //各頂点のボーンの影響度を処理
    UINT boneCount = meshReader.ReadUInt32();
    for (UINT i = 0; i < boneCount; i++) {
        //ボーン名からindexを取得
        char* boneNameBuf = meshReader.ReadBytes(meshReader.ReadByte());
        std::string boneName = boneNameBuf;
        delete[] boneNameBuf;

        UINT boneIndex = m_boneManager.m_boneMapping[boneName];

        //影響を受ける頂点の数
        UINT weightCount = meshReader.ReadUInt32();
        for (UINT j = 0; j < weightCount; j++) {
            UINT vertexID = meshReader.ReadUInt32();
            float weight = meshReader.ReadFloat();

            //頂点を取得
            Vertex& vertex = vertices[vertexID];

            //空いているボーンウェイトスロットを探す。 (最大4つのボーンから影響を受ける)
            if (vertex.boneWeights.x == 0.0f) {
                vertex.boneIDs[0] = boneIndex;
                vertex.boneWeights.x = weight;
            }
            else if (vertex.boneWeights.y == 0.0f) {
                vertex.boneIDs[1] = boneIndex;
                vertex.boneWeights.y = weight;
            }
            else if (vertex.boneWeights.z == 0.0f) {
                vertex.boneIDs[2] = boneIndex;
                vertex.boneWeights.z = weight;
            }
            else if (vertex.boneWeights.w == 0.0f) {
                vertex.boneIDs[3] = boneIndex;
                vertex.boneWeights.w = weight;
            }
            else {
                //4つ以上のボーンウェイトはサポート外
                //printf("頂点 %d は 4 つ以上のボーンウェイトを持っています。\n", vertexID);
            }
        }
    }

    //頂点数を保存
    humanoidMesh.vertexCount = static_cast<UINT>(vertices.size());

    //シェーダーに必要なバッファを生成
    Model::CreateBuffer(humanoidMesh.pMesh, vertices, indices, sizeof(Vertex), meshIndex);
    CreateBuffer(humanoidMesh);
}

void Character::CreateBuffer(HumanoidMesh& humanoidMesh)
{
    //ヒープ設定
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    //ボーン情報のリソースを作成
    CD3DX12_RESOURCE_DESC boneBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(XMMATRIX) * m_boneManager.m_boneInfos.size());
    HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &boneBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_boneMatricesBuffer));
    if (FAILED(hr)) {
        printf("ボーンバッファの生成に失敗しました。\n");
    }

    m_boneMatricesBuffer->Map(0, nullptr, &m_pBoneMatricesMap);

    //頂点数とシェイプキー数を保持
    Contents contents{};
    contents.vertexCount = humanoidMesh.vertexCount;
    contents.shapeCount = static_cast<UINT>(humanoidMesh.shapeWeights.size());
    void* pContentsBuffer;
    humanoidMesh.pMesh->meshData->contentsBuffer->Map(0, nullptr, &pContentsBuffer);
    if (pContentsBuffer)
        memcpy(pContentsBuffer, &contents, sizeof(Contents));
    humanoidMesh.pMesh->meshData->contentsBuffer->Unmap(0, nullptr);
}

void Character::CreateHumanoidMeshBuffer(HumanoidMesh& humanoidMesh)
{
    //ヒープ設定
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    //シェイプキーの情報を保持する用のリソースを作成
    if (humanoidMesh.shapeWeights.size() > 0) {
        //シェイプキーのウェイト情報を保持するリソースを作成
        UINT64 shapeWeightsSize = sizeof(float) * humanoidMesh.shapeWeights.size();
        D3D12_RESOURCE_DESC shapeWeightsBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(shapeWeightsSize);
        HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &shapeWeightsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&humanoidMesh.pMesh->shapeWeightsBuffer));
        if (FAILED(hr)) {
            printf("シェイプキーウェイトバッファの生成に失敗しました。%ld\n", hr);
            HRESULT removedReason = m_pDevice->GetDeviceRemovedReason();
            printf("Device removed reason:%ld\n", removedReason);
        }

        //ウェイト情報の初期値を入れる (Update関数で常に更新される)
        humanoidMesh.pMesh->shapeWeightsBuffer->Map(0, nullptr, &humanoidMesh.pMesh->pShapeWeightsMapped);
        memcpy(humanoidMesh.pMesh->pShapeWeightsMapped, humanoidMesh.shapeWeights.data(), shapeWeightsSize);
    }
}

void Character::CreateShapeDeltasTexture(bool bClearDeltas)
{
    //メインスレッドで必要なアップロードバッファを作成
    //std::vector<ID3D12Resource*> shapeBuffers(m_humanoidMeshes.size());
    shapeBuffers.resize(m_humanoidMeshes.size());

    for (UINT i = 0; i < static_cast<UINT>(m_humanoidMeshes.size()); i++) {
        HumanoidMesh& humanoidMesh = m_humanoidMeshes[i];
        size_t vertexCount = humanoidMesh.vertexCount;
        size_t shapeCount = humanoidMesh.shapeWeights.size();

        if (shapeCount <= 0) {
            shapeBuffers[i] = nullptr;
            continue;
        }

        UINT maxWidth = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION; //横幅の最大 = 16384
        UINT width = (vertexCount > maxWidth) ? maxWidth : static_cast<UINT>(vertexCount);
        UINT height = static_cast<UINT>(((vertexCount + width - 1) / width) * shapeCount);

        size_t rowPitch = (width * sizeof(XMFLOAT4) + 255) & ~255; //256の倍数になるように

        shapeBuffers[i] = g_resourceCopy->CreateUploadBuffer(rowPitch * height);

        if (humanoidMesh.pMesh->meshData->shapeDeltasBuffer) {
            if (shapeBuffers[i]) {
                shapeBuffers[i]->Release();
                shapeBuffers[i] = nullptr;
            }
            continue;
        }

        //シェイプキー用のリソースを作成
        CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT, static_cast<UINT>(width), static_cast<UINT>(height), 1, 1);
        CD3DX12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COPY_DEST;
        HRESULT result = m_pDevice->CreateCommittedResource(&texHeapProp, D3D12_HEAP_FLAG_NONE, &resDesc, state, nullptr, IID_PPV_ARGS(&humanoidMesh.pMesh->meshData->shapeDeltasBuffer));
        if (FAILED(result)) {
            printf("シェイプキー用のリソ−スの作成に失敗しました。: エラーコード = %1x\n", result);
        }
    }

    //別スレッドでアップロードバッファにデータをコピーし、GPUに送信
    std::thread([=] {
        for (UINT i = 0; i < static_cast<UINT>(m_humanoidMeshes.size()); i++) {
            HumanoidMesh& humanoidMesh = m_humanoidMeshes[i];

            if (!shapeBuffers[i]) {
                if (bClearDeltas) {
                    humanoidMesh.shapeDeltas.clear();
                }
                continue;
            }


            //必要なサイズを計算
            size_t vertexCount = humanoidMesh.vertexCount;
            size_t shapeCount = humanoidMesh.shapeWeights.size();

            if (shapeCount <= 0) {
                if (bClearDeltas) {
                    humanoidMesh.shapeDeltas.clear();
                }
                continue;
            }

            g_resourceCopy->BeginCopyResource();

            UINT maxWidth = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION; //横幅の最大 = 16384
            UINT width = (vertexCount > maxWidth) ? maxWidth : static_cast<UINT>(vertexCount);
            UINT height = static_cast<UINT>(((vertexCount + width - 1) / width) * shapeCount);

            size_t rowPitch = (width * sizeof(XMFLOAT4) + 255) & ~255; //256の倍数になるように
            size_t tempWidth = rowPitch / sizeof(XMFLOAT4);
            std::vector<XMFLOAT4> textureData(tempWidth * height, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f)); //初期化

            //シェイプキー情報をテクスチャに変換
            for (size_t shapeID = 0; shapeID < shapeCount; shapeID++) {
                for (size_t vertexID = 0; vertexID < vertexCount; vertexID++) {
                    size_t x = vertexID % tempWidth;
                    size_t y = (vertexID / tempWidth) * shapeCount + shapeID; //縦位置

                    //範囲チェック
                    if (y >= height) {
                        continue;
                    }

                    XMFLOAT3 delta = humanoidMesh.shapeDeltas[shapeID * vertexCount + vertexID];
                    textureData[y * tempWidth + x] = XMFLOAT4(delta.x, delta.y, delta.z, 0.0f);
                }
            }

            void* mappedData = nullptr;
            HRESULT hr = shapeBuffers[i]->Map(0, nullptr, &mappedData);
            if (FAILED(hr)) {
                printf("UploadBufferのマップに失敗しました。\n");
                return;
            }
            memcpy(mappedData, textureData.data(), rowPitch * height);
            shapeBuffers[i]->Unmap(0, nullptr);

            //コピー先を設定
            D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
            dstLocation.pResource = humanoidMesh.pMesh->meshData->shapeDeltasBuffer.Get();
            dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dstLocation.SubresourceIndex = 0;

            //コピー元を設定
            D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
            srcLocation.pResource = shapeBuffers[i];
            srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            srcLocation.PlacedFootprint.Footprint.Width = width;
            srcLocation.PlacedFootprint.Footprint.Height = height;
            srcLocation.PlacedFootprint.Footprint.Depth = 1;
            srcLocation.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(rowPitch);
            srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            g_resourceCopy->GetCommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

            //リソースバリアでSTATEを変更
            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(humanoidMesh.pMesh->meshData->shapeDeltasBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            g_resourceCopy->GetCommandList()->ResourceBarrier(1, &barrier);
            g_resourceCopy->EndCopyResource();

            if (bClearDeltas) {
                humanoidMesh.shapeDeltas.clear();
            }

            if (shapeBuffers[i]) {
                shapeBuffers[i]->Release();
                shapeBuffers[i] = nullptr;
            }
        }
        shapeBuffers.clear();
        }).detach();
}

void Character::CalculateBoneTransforms(const aiNode* node, const XMMATRIX& parentTransform)
{
    //ボーンがboneMapに存在するかチェック
    Bone* pBone = m_boneManager.GetBone(node->mName.C_Str());
    XMMATRIX nodeTransform = XMMatrixIdentity();

    if (pBone) {
        //ボーンのローカル変換行列を取得
        nodeTransform = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&node->mTransformation)));
        //printf("NodeName = %s, x=%f, y=%f, z=%f\n", node->mName.C_Str(), nodeTransform.r[3].m128_f32[0], nodeTransform.r[3].m128_f32[1], nodeTransform.r[3].m128_f32[2]);
    }

    //親ボーンの変換行列との合成（親から子への変換）
    XMMATRIX globalTransform = nodeTransform * parentTransform;

    //ボーンのワールド空間での変換行列を保存（Sphereを置くための位置として使用）
    if (pBone) {
        pBone->SetBoneOffset(globalTransform);
    }

    //子ノードに対して再帰的に処理を実行
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        CalculateBoneTransforms(node->mChildren[i], globalTransform);
    }
}

void Character::LoadBones(aiMesh* mesh, std::vector<Vertex>& vertices)
{
    for (UINT i = 0; i < mesh->mNumBones; i++) {
        aiBone* bone = mesh->mBones[i];
        UINT boneIndex = 0;

        std::string boneName = bone->mName.C_Str();

        //ボーンがまだ登録されていない場合、マッピングを追加
        if (!m_boneManager.Exist(boneName)) {
            //追加するボーンのインデックスを取得
            boneIndex = static_cast<UINT>(m_boneManager.m_boneInfos.size());

            //ボーンを作成
            Bone* pBoneChild = m_boneManager.AddBone(boneName, boneIndex);

            //手や足は左右を判定 (一旦手の判定をし、足の場合はLoadBoneFamily内で変更)
            if (boneName[0] == 'L' || boneName.at(boneName.size() - 1) == 'L') {
                pBoneChild->m_boneType = BONETYPE_LEFT_ARM;
            }
            else if (boneName[0] == 'R' || boneName.at(boneName.size() - 1) == 'R') {
                pBoneChild->m_boneType = BONETYPE_RIGHT_ARM;
            }
            //髪はボーンの回転軸が異なる (Unityと同じように変更)
            else if (boneName.find("_L") != std::string::npos || boneName.find("L.") != std::string::npos) {
                pBoneChild->m_boneType = BONETYPE_LEFT_HAIR;
            }
            else if (boneName.find("_R") != std::string::npos || boneName.find("R.") != std::string::npos) {
                pBoneChild->m_boneType = BONETYPE_RIGHT_HAIR;
            }

            //目のボーンは左右の区別をしない
            else if (boneName.find("Eye") != std::string::npos || boneName.find("eye") != std::string::npos) {
                pBoneChild->m_boneType = BONETYPE_DEFAULT;
            }
        }
        else {
            //同名のボーンが存在する場合それを使用 (異なるメッシュ間で同名のボーンの場合、ほとんどが共有されているため)
            boneIndex = m_boneManager.m_boneMapping[bone->mName.C_Str()];
        }

        //ボーンの影響を受ける頂点の数ぶん繰り返す
        for (UINT j = 0; j < bone->mNumWeights; j++) {
            //頂点インデックス
            UINT vertexID = bone->mWeights[j].mVertexId;
            //その頂点がどのくらい影響を受けるか
            float weight = bone->mWeights[j].mWeight;

            //頂点を取得
            Vertex& vertex = vertices[vertexID];

            //空いているボーンウェイトスロットを探す。 (最大4つのボーンから影響を受ける)
            if (vertex.boneWeights.x == 0.0f) {
                vertex.boneIDs[0] = boneIndex;
                vertex.boneWeights.x = weight;
            }
            else if (vertex.boneWeights.y == 0.0f) {
                vertex.boneIDs[1] = boneIndex;
                vertex.boneWeights.y = weight;
            }
            else if (vertex.boneWeights.z == 0.0f) {
                vertex.boneIDs[2] = boneIndex;
                vertex.boneWeights.z = weight;
            }
            else if (vertex.boneWeights.w == 0.0f) {
                vertex.boneIDs[3] = boneIndex;
                vertex.boneWeights.w = weight;
            }
            else {
                //4つ以上のボーンウェイトはサポート外
                //printf("頂点 %d は 4 つ以上のボーンウェイトを持っています。\n", vertexID);
            }
        }
    }
}

void Character::LoadBones(BinaryReader& br)
{
    UINT boneBufferOriginalSize = br.ReadUInt32();
	UINT boneBufferCompressedSize = br.ReadUInt32();
	char* compressedBuffer = br.ReadBytes(boneBufferCompressedSize);

	//圧縮されているボーン情報を解凍
    std::vector<char> boneBuffer;
    BinaryDecompress(boneBuffer, boneBufferOriginalSize, compressedBuffer, boneBufferCompressedSize);

    delete[] compressedBuffer;

	//解凍したバッファを読み込む
	BinaryReader boneReader(boneBuffer);

    UINT boneCount = boneReader.ReadUInt32();
    for (UINT i = 0; i < boneCount; i++) {
        UINT boneIndex = static_cast<UINT>(m_boneManager.m_bones.size());

        //ボーン名
        char* boneNameBuf = boneReader.ReadBytes(boneReader.ReadByte());
        std::string boneName = boneNameBuf;
        delete[] boneNameBuf;

        //ボーンの初期位置
        XMMATRIX matrix = boneReader.ReadMatrix();

        Bone* pBone = m_boneManager.AddBone(boneName, boneIndex);
        pBone->m_boneType = (BoneType)boneReader.ReadSByte();

        //m_boneManager.m_finalBoneTransforms[boneName] = boneReader.ReadMatrix();
        pBone->SetBoneOffset(boneReader.ReadMatrix());
    }

    for (UINT i = 0; i < boneCount; i++) {
        Bone& bone = m_boneManager.m_bones[i];
        UINT parentBoneIndex = boneReader.ReadUInt32();
        if (m_boneManager.m_bones.size() > parentBoneIndex) {
            bone.SetParentBone(&m_boneManager.m_bones[parentBoneIndex], parentBoneIndex);
        }

        UINT childBoneCount = boneReader.ReadByte();
        for (UINT j = 0; j < childBoneCount; j++) {
            UINT childIndex = boneReader.ReadUInt32();
            bone.AddChildBone(&m_boneManager.m_bones[childIndex], childIndex);
        }
    }
}

//ボーンの親子関係を取得
void Character::LoadBoneFamily(const aiNode* node)
{
    //ノード(Armatureボーン)の上から下へ実行される

    //ノード名のボーンが存在
    if (m_boneManager.Exist(node->mName.C_Str())) {
        UINT boneIndex = m_boneManager.m_boneMapping[node->mName.C_Str()];
        //ノードの子を列挙
        for (UINT i = 0; i < node->mNumChildren; i++) {
            //子ノードの名前のボーンが存在
            if (m_boneManager.m_boneMapping.find(node->mChildren[i]->mName.C_Str()) != m_boneManager.m_boneMapping.end()) {
                UINT childIndex = m_boneManager.m_boneMapping[node->mChildren[i]->mName.C_Str()];

                //子ボーンと親ボーンを設定
                m_boneManager.m_bones[boneIndex].AddChildBone(&m_boneManager.m_bones[childIndex], childIndex);
                m_boneManager.m_bones[childIndex].SetParentBone(&m_boneManager.m_bones[boneIndex], boneIndex);

                std::string boneName = m_boneManager.m_bones[childIndex].GetBoneName();
                //足の場合BONETYPE_LEFT(RIGHT)_LEGに変更 (LoadBones()で足もBONETYPE_LEFT_ARMと指定されるため)
                if (boneName.find("Leg") != std::string::npos || boneName.find("leg") != std::string::npos) {
                    if (m_boneManager.m_bones[childIndex].m_boneType == BONETYPE_LEFT_ARM) {
                        m_boneManager.m_bones[childIndex].m_boneType = BONETYPE_LEFT_LEG;
                    }
                    else if (m_boneManager.m_bones[childIndex].m_boneType == BONETYPE_RIGHT_ARM) {
                        m_boneManager.m_bones[childIndex].m_boneType = BONETYPE_RIGHT_LEG;
                    }
                    continue;
                }

                if (m_boneManager.m_bones[boneIndex].GetBoneName().find("shoulder") != std::string::npos) {
                    continue;
                }

                //子ボーンは親ボーンの種類を継承
                if (m_boneManager.m_bones[boneIndex].m_boneType != BONETYPE_DEFAULT) {
                    m_boneManager.m_bones[childIndex].m_boneType = m_boneManager.m_bones[boneIndex].m_boneType;
                }
            }
        }
    }

    //子ボーンも同様に設定
    for (UINT i = 0; i < node->mNumChildren; i++) {
        LoadBoneFamily(node->mChildren[i]);
    }
}

void Character::LoadShapeKey(const aiMesh* mesh, std::vector<Vertex>& vertices, HumanoidMesh& humanoidMesh)
{
    //メッシュに含まれるシェイプキー一覧
    for (UINT i = 0; i < mesh->mNumAnimMeshes; i++) {
        aiAnimMesh* animMesh = mesh->mAnimMeshes[i];

        std::string shapeName = UTF8ToShiftJIS(animMesh->mName.C_Str());

        //同名のシェイプキーが存在すれば無視 (ほぼないけど)
        if (humanoidMesh.shapeMapping.find(shapeName) == humanoidMesh.shapeMapping.end()) {
            UINT shapeIndex = static_cast<UINT>(humanoidMesh.shapeWeights.size());

            humanoidMesh.shapeWeights.push_back(0.0f);
            humanoidMesh.shapeMapping[shapeName] = shapeIndex;

            for (UINT j = 0; j < animMesh->mNumVertices; j++) {
                aiVector3D& vec = animMesh->mVertices[j];
                //シェイプキーの、100%のときのその頂点の位置
                XMFLOAT3 shapePos = XMFLOAT3(vec.x, vec.y, vec.z);
                //元の位置を引く
                shapePos.x -= vertices[j].position.x;
                shapePos.y -= vertices[j].position.y;
                shapePos.z -= vertices[j].position.z;
                humanoidMesh.shapeDeltas.push_back(shapePos);
            }
        }
    }
}

void Character::LoadHumanoidMesh(BinaryReader& br)
{
    UINT humanoidBufferOriginalSize = br.ReadUInt32();
    UINT humanoidBufferCompressedSize = br.ReadUInt32();
    char* compressedBuffer = br.ReadBytes(humanoidBufferCompressedSize);

    if (s_sharedHumanoidMeshes.find(m_modelFile) != s_sharedHumanoidMeshes.end()) {
        HumanoidList& humanoidList = s_sharedHumanoidMeshes[m_modelFile];
        humanoidList.refCount++;
        for (UINT i = 0; i < humanoidList.humanoidMeshCount; i++) {
            HumanoidMesh humanoidMesh;
            humanoidMesh.pMesh = new Mesh();
            humanoidMesh.pMesh->meshName = humanoidList.meshNames[i];
            humanoidMesh.shapeMapping = humanoidList.shapeMappings[i];
            for (UINT j = 0; j < humanoidList.shapeCounts[i]; j++) {
                humanoidMesh.shapeWeights.push_back(0.0f);
            }

            m_humanoidMeshes.push_back(humanoidMesh);
        }

        delete[] compressedBuffer;
        return;
    }

    //圧縮されているシェイプ情報を解凍
    std::vector<char> humanoidBuffer;
    BinaryDecompress(humanoidBuffer, humanoidBufferOriginalSize, compressedBuffer, humanoidBufferCompressedSize);

    delete[] compressedBuffer;

    //解凍したバッファを読み込む
    BinaryReader humanoidReader(humanoidBuffer);
    UINT humanoidMeshCount = humanoidReader.ReadUInt32();

    HumanoidList humanoidList{};
    humanoidList.humanoidMeshCount = humanoidMeshCount;
    humanoidList.shapeMappings.resize(humanoidMeshCount);
    humanoidList.meshNames.resize(humanoidMeshCount);
    humanoidList.shapeCounts.resize(humanoidMeshCount);

    for (UINT i = 0; i < humanoidMeshCount; i++) {
        HumanoidMesh humanoidMesh;
        humanoidMesh.pMesh = new Mesh();

        //メッシュ名
        char* meshNameBuf = humanoidReader.ReadBytes(humanoidReader.ReadByte());
        humanoidMesh.pMesh->meshName = meshNameBuf;
        humanoidList.meshNames[i] = meshNameBuf;
        delete[] meshNameBuf;

        UINT shapeMappingCount = humanoidReader.ReadUInt32();
        humanoidList.shapeCounts[i] = shapeMappingCount;
        for (UINT j = 0; j < shapeMappingCount; j++) {
            //シェイプキー名
            char* shapeNameBuf = humanoidReader.ReadBytes(humanoidReader.ReadByte());
            std::string shapeName = shapeNameBuf;

            //.区切りで同じ文字が続いている場合削除
            std::vector<std::string> sprits = GetSprits(shapeName, '.');
            if (sprits.size() == 2) {
                shapeName = sprits[0];
            }

            delete[] shapeNameBuf;

            //シェイプキーのインデックス
            UINT shapeIndex = humanoidReader.ReadUInt32();

            humanoidMesh.shapeWeights.push_back(0.0f);
            humanoidMesh.shapeMapping[shapeName] = shapeIndex;
            humanoidList.shapeMappings[i].emplace(shapeName, shapeIndex);
        }
        //シェイプキーの、100%のときのその頂点の位置
        UINT shapeDeltaCount = humanoidReader.ReadUInt32();
        UINT loadDeltaCount = humanoidReader.ReadUInt32();
		humanoidMesh.shapeDeltas.resize(shapeDeltaCount);
        for (UINT j = 0; j < loadDeltaCount; j++) {
			UINT index = humanoidReader.ReadUInt32();
			humanoidMesh.shapeDeltas[index].x = humanoidReader.ReadFloat();
			humanoidMesh.shapeDeltas[index].y = humanoidReader.ReadFloat();
			humanoidMesh.shapeDeltas[index].z = humanoidReader.ReadFloat();
        }

        m_humanoidMeshes.push_back(humanoidMesh);
    }
    s_sharedHumanoidMeshes.emplace(m_modelFile, humanoidList);
}

void Character::SetTexture(Mesh* pMesh, const std::string nameOnly)
{
    //std::string dir = "Resource\\Model\\Milltina\\";
    std::string dir = "Resource\\Model\\";

    std::string texPath = dir + nameOnly;
    //std::string normalPath = dir + "Skin_Normal Map.png";

    //マテリアルを作成
    bool bExist;
    pMesh->pMaterial = m_pMaterialManager->AddMaterialWithShapeData(texPath, bExist, pMesh->meshData->shapeDataIndex);
    if (!bExist) {
        pMesh->pMaterial->SetMainTexture(texPath);
    }
}

//ボーンの位置を更新
void Character::UpdateBonePosition(std::string boneName, XMFLOAT3& position)
{
    if (!m_boneManager.Exist(boneName)) {
        return;
    }

    UINT boneIndex = m_boneManager.m_boneMapping[boneName];

    m_boneManager.m_bones[boneIndex].m_position = position;
}

//ボーンの回転を変更
void Character::UpdateBoneRotation(std::string boneName, XMFLOAT4& rotation)
{
    if (!m_boneManager.Exist(boneName)) {
        return;
    }

    UINT boneIndex = m_boneManager.m_boneMapping[boneName];

    m_boneManager.m_bones[boneIndex].m_rotation = rotation;
}

//ボーンのスケールを変更
void Character::UpdateBoneScale(std::string boneName, XMFLOAT3& scale)
{
    if (!m_boneManager.Exist(boneName)) {
        return;
    }

    UINT boneIndex = m_boneManager.m_boneMapping[boneName];

    m_boneManager.m_bones[boneIndex].m_scale = scale;
}

//シェイプキーのウェイトを更新
void Character::SetShapeWeight(const std::string shapeName, float weight)
{
    if (weight < 0.0f)
        weight = 0.0f;
    else if (weight > 1.2f)
        weight = 1.2f;

    for (HumanoidMesh& mesh : m_humanoidMeshes) {
        if (mesh.shapeMapping.find(shapeName) == mesh.shapeMapping.end()) {
            continue;
        }

        UINT shapeIndex = mesh.shapeMapping[shapeName];
        mesh.shapeWeights[shapeIndex] = weight;
        mesh.bChangeShapeValue = true;
    }
}

void Character::UpdateShapeKeys()
{
    for (HumanoidMesh& mesh : m_humanoidMeshes) {
        //シェイプバッファに送信
        if (mesh.pMesh->shapeWeightsBuffer && mesh.bChangeShapeValue) {
            memcpy(mesh.pMesh->pShapeWeightsMapped, mesh.shapeWeights.data(), sizeof(float) * mesh.shapeWeights.size()); //各シェイプキーのウェイトをコピー
            mesh.bChangeShapeValue = false;
        }
    }
}

void Character::UpdateAnimation()
{
    //アニメーションが存在しない
    if (m_nowAnimationIndex < 0 || m_characterAnimations.size() <= m_nowAnimationIndex) {
        return;
    }

    //キーフレームが存在しない
    if (m_characterAnimations[m_nowAnimationIndex].pAnimation->m_frames.size() <= 0) {
        return;
    }

    //アニメーション時間を更新
    m_nowAnimationTime += *m_pFrameTime * m_animationSpeed;

    //現在のアニメーション時間のフレームを取得
    CharacterAnimationFrame* pFrame = m_characterAnimations[m_nowAnimationIndex].pAnimation->GetCharacterFrame(m_nowAnimationTime, &m_characterAnimations[m_nowAnimationIndex].beforeFrameIndex);

    //フレームが存在しなければ処理を終了 (主にアニメーションが読み込まれていない場合)
    if (!pFrame) {
        return;
    }

    m_boneManager.m_armatureBone.m_position = pFrame->armatureAnimation.position;
    m_boneManager.m_armatureBone.m_rotation = pFrame->armatureAnimation.rotation;

    //ボーンアニメーション
    for (UINT i = 0; i < pFrame->boneAnimations.size(); i++) {
        std::string boneName = m_characterAnimations[m_nowAnimationIndex].pAnimation->m_boneMapping[i];
        UpdateBonePosition(boneName, pFrame->boneAnimations[i].position);
        UpdateBoneRotation(boneName, pFrame->boneAnimations[i].rotation);
    }

    //シェイプキーのアニメーション
    for (UINT i = 0; i < m_characterAnimations[m_nowAnimationIndex].pAnimation->m_shapeNames.size(); i++) {
        std::string& shapeName = m_characterAnimations[m_nowAnimationIndex].pAnimation->m_shapeNames[i];
        if (pFrame->shapeAnimations.size() > i) {
            SetShapeWeight(shapeName, pFrame->shapeAnimations[i]);
        }
    }

    //再生中のフレームが最後のフレームだった場合最初に戻す
    if (m_characterAnimations[m_nowAnimationIndex].pAnimation->IsLastFrame(pFrame)) {
        m_nowAnimationTime = 0.0f;
    }
}

Character::HumanoidMesh::HumanoidMesh()
    : pMesh(nullptr)
    , vertexCount(0)
    , bChangeShapeValue(true)
{
}
