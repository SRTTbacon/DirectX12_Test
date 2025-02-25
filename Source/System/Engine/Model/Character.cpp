#include "Character.h"

static XMVECTOR ExtractEulerAngles(const XMMATRIX& matrix) {
    float pitch, yaw, roll;

    //Y軸方向の回転（yaw）
    yaw = asinf(-matrix.r[2].m128_f32[0]);

    if (cosf(yaw) > 0.0001f) {
        //X軸とZ軸方向の回転（pitch と roll）
        pitch = atan2f(matrix.r[2].m128_f32[1], matrix.r[2].m128_f32[2]);
        roll = atan2f(matrix.r[1].m128_f32[0], matrix.r[0].m128_f32[0]);
    }
    else {
        //Y軸が 90度または -90度の時は、Gimbal Lockに対応
        pitch = atan2f(-matrix.r[0].m128_f32[2], matrix.r[1].m128_f32[1]);
        roll = 0.0f;
    }

    return XMVectorSet(pitch, yaw, roll, 0.0f); //ラジアン角で出力
}

Character::Character(const std::string modelFile, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const Camera* pCamera, DirectionalLight* pDirectionalLight,
    ID3D12Resource* pShadowMapBuffer)
    : Model(pDevice, pCommandList, pCamera, pDirectionalLight, pShadowMapBuffer)
    , m_animationSpeed(1.0f)
    , m_nowAnimationTime(0.0f)
    , m_nowAnimationIndex(-1)
    , bHCSFile(false)
{
    DWORD startTime = timeGetTime();
    m_modelFile = modelFile;
    m_modelType = ModelType_Character;

    CreateConstantBuffer();

    DWORD tempTime = timeGetTime() - startTime;
    printf("CreateConstantBuffer - %dms\n", tempTime);

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

    //ルートシグネチャとパイプラインステートを初期化
	m_pRootSignature = new RootSignature(m_pDevice, ShaderKinds::BoneShader);
	m_pPipelineState = new PipelineState(m_pDevice, m_pRootSignature);
    printf("PipelineState - %dms\n", timeGetTime() - tempTime);

    m_boneManager.UpdateBoneMatrix();

    DWORD loadTime = timeGetTime() - startTime;
	printf("モデル:%sをロードしました。 %dms\n", modelFile.c_str(), loadTime);

    g_Engine->EndRender();
    g_Engine->BeginRender();
}

Character::~Character()
{
    Model::~Model();
}

UINT Character::AddAnimation(Animation animation)
{
    m_animations.push_back(animation);
	//アニメーションが1つしかない場合は、現在のアニメーションインデックスを0にする
    if (m_animations.size() == 1) {
        m_nowAnimationIndex = 0;
    }
    return (unsigned int)(m_animations.size() - 1);
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
    void* pData;
    HRESULT hr = m_boneMatricesBuffer->Map(0, nullptr, &pData);
    if (SUCCEEDED(hr))
    {
        memcpy(pData, m_boneManager.m_boneInfos.data(), sizeof(XMMATRIX) * m_boneManager.m_boneInfos.size()); //ボーンマトリックスをコピー
        m_boneMatricesBuffer->Unmap(0, nullptr);
    }

    Model::LateUpdate(backBufferIndex);

    //キャラクターはルートモーションを考慮
    XMFLOAT3 armaturePos = m_boneManager.m_armatureBone.m_position;
    armaturePos.x = -armaturePos.x;
    XMFLOAT3 hipPos = m_boneManager.GetBone("Hips")->m_position;
    hipPos.z = -hipPos.z;
    XMMATRIX& m = m_boneManager.m_boneInfos[m_boneManager.GetBone("Hips")->GetBoneIndex()];
    //XMFLOAT3 tempPos = armaturePos + m_position + m_boneManager.GetBone("Hips")->m_position;
    //XMFLOAT3 tempPos = armaturePos + m_position + hipPos;
    XMFLOAT3 tempPos = XMFLOAT3(m.r[3].m128_f32[0], m.r[3].m128_f32[1], m.r[3].m128_f32[2]) + m_position;
    XMVECTOR objPos = XMLoadFloat3(&tempPos);
    XMVECTOR camPos = m_pCamera->m_eyePos;

    // カメラ位置からオブジェクト位置までのユークリッド距離をそのまま m_depth に設定
    m_depth = XMVectorGetX(XMVector3Length(XMVectorSubtract(objPos, camPos)));

    if (m_nowAnimationIndex != -1) {
        printf("AnimFile = %s : %f, %f, %f, Length -> %f\n", m_animations[m_nowAnimationIndex].GetFilePath().c_str(), tempPos.x, tempPos.y, tempPos.z, m_depth);
    }
    else {
        printf("%f, %f, %f, Length -> %f\n", tempPos.x, tempPos.y, tempPos.z, m_depth);
    }
}

void Character::LoadFBX(const std::string& fbxFile)
{
    Assimp::Importer importer;

    //モデル読み込み時のフラグ。メッシュのポリゴンはすべて三角形にし、ボーンが存在する場合は影響を受けるウェイトを4つまでにする
    UINT flag = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
        aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights;
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

    //マテリアルの読み込み
    m_pDescriptorHeap = new DescriptorHeap(m_pDevice, scene->mNumMeshes, CHARACTER_DISCRIPTOR_HEAP_SIZE, ShadowSizeHigh);

    for (size_t i = 0; i < scene->mNumMeshes; i++)
    {
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
            Texture2D* pTexture = Texture2D::GetColor(1.0f, 1.0f, 1.0f);
            m_pDescriptorHeap->SetMainTexture(pTexture->Resource(), pTexture->Resource(), m_pShadowMapBuffer, m_meshes[i]->shapeDeltasBuffer.Get());
            m_textures.push_back(pTexture);
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
    printf("LoadBone - %dms\n", endTime - startTime);
    startTime = endTime;
    LoadHumanoidMesh(br);
    endTime = timeGetTime();
    printf("LoadHumanoidMesh - %dms\n", endTime - startTime);
    startTime = endTime;
    UINT meshCount = br.ReadUInt32();
    for (UINT i = 0; i < meshCount; i++) {
        ProcessMesh(br, m_humanoidMeshes[i]);
        m_meshes.push_back(m_humanoidMeshes[i].pMesh);
    }

    endTime = timeGetTime();
    printf("ProcessMesh - %dms\n", endTime - startTime);
    startTime = endTime;

    for (HumanoidMesh& humanoidMesh : m_humanoidMeshes) {
        CreateHumanoidMeshBuffer(humanoidMesh);

        //HCSファイルの場合はシェイプキーをクリア
        humanoidMesh.shapeDeltas.clear();
    }
    endTime = timeGetTime();
    printf("CreateHumanoidMeshBuffer - %dms\n", endTime - startTime);
    startTime = endTime;

    //マテリアルの読み込み
    m_pDescriptorHeap = new DescriptorHeap(m_pDevice, meshCount, CHARACTER_DISCRIPTOR_HEAP_SIZE, ShadowSizeHigh);

    for (UINT i = 0; i < meshCount; i++) {
        bool bExistTexture = br.ReadBoolean();

        //メッシュのマテリアルを取得する
        if (bExistTexture) {
            char* nameBuf = br.ReadBytes(br.ReadByte());
            std::string nameOnly = nameBuf;
            delete[] nameBuf;

            SetTexture(m_meshes[i], nameOnly);
        }
        else {
			//テクスチャが存在しない場合は白単色テクスチャを使用
            Texture2D* pTexture = Texture2D::GetColor(1.0f, 1.0f, 1.0f);
            m_pDescriptorHeap->SetMainTexture(pTexture->Resource(), pTexture->Resource(), m_pShadowMapBuffer, m_meshes[i]->shapeDeltasBuffer.Get());
			m_textures.push_back(pTexture);
        }
    }

    bHCSFile = true;

    endTime = timeGetTime();
    printf("SetTexture - %dms\n", endTime - startTime);
}

void Character::ProcessNode(const aiScene* pScene, const aiNode* pNode)
{
    //ノードにメッシュが含まれていたら読み込む
    for (UINT j = 0; j < pNode->mNumMeshes; j++) {
        aiMesh* mesh = pScene->mMeshes[pNode->mMeshes[j]];
        HumanoidMesh humanoidMesh{};
        Mesh* pMesh = ProcessMesh(mesh, humanoidMesh);
        pMesh->meshName = UTF8ToShiftJIS(pNode->mName.C_Str());
        m_humanoidMeshes.push_back(humanoidMesh);
        m_meshes.push_back(pMesh);
    }

    for (UINT i = 0; i < pNode->mNumChildren; i++) {
        aiNode* childNode = pNode->mChildren[i];
        ProcessNode(pScene, childNode);
    }
}

Mesh* Character::ProcessMesh(aiMesh* mesh, HumanoidMesh& humanoidMesh) {
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
    CreateBuffer(meshData, vertices, indices, humanoidMesh);

    return meshData;
}

void Character::ProcessMesh(BinaryReader& br, HumanoidMesh& humanoidMesh)
{
    UINT meshBufferOriginalSize = br.ReadUInt32();
    UINT meshBufferCompressedSize = br.ReadUInt32();
    char* compressedBuffer = br.ReadBytes(meshBufferCompressedSize);

    //圧縮されているボーン情報を解凍
    std::vector<char> meshBuffer;
    BinaryDecompress(meshBuffer, meshBufferOriginalSize, compressedBuffer, meshBufferCompressedSize);

    delete[] compressedBuffer;

    //解凍したバッファを読み込む
    BinaryReader meshReader(meshBuffer);

    std::vector<Vertex> vertices;
    std::vector<UINT> indices;

    //頂点の処理
    UINT vertexCount = meshReader.ReadUInt32();
    for (UINT i = 0; i < vertexCount; i++) {
        Vertex vertex{};

        vertex.position.x = meshReader.ReadFloat();
        vertex.position.y = meshReader.ReadFloat();
        vertex.position.z = meshReader.ReadFloat();
        vertex.normal.x = meshReader.ReadFloat();
        vertex.normal.y = meshReader.ReadFloat();
        vertex.normal.z = meshReader.ReadFloat();
        vertex.texCoords.x = meshReader.ReadFloat();
        vertex.texCoords.y = meshReader.ReadFloat();
		vertex.tangent.x = meshReader.ReadFloat();
		vertex.tangent.y = meshReader.ReadFloat();
		vertex.tangent.z = meshReader.ReadFloat();
		vertex.bitangent.x = meshReader.ReadFloat();
		vertex.bitangent.y = meshReader.ReadFloat();
		vertex.bitangent.z = meshReader.ReadFloat();

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
    CreateBuffer(humanoidMesh.pMesh, vertices, indices, humanoidMesh);
}

void Character::CreateBuffer(Mesh* pMesh, std::vector<Vertex>& vertices, std::vector<UINT>& indices, HumanoidMesh& humanoidMesh)
{
    Model::CreateBuffer(pMesh, vertices, indices, sizeof(Vertex));

    //ヒープ設定
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    //ボーン情報のリソースを作成
    CD3DX12_RESOURCE_DESC boneBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(XMMATRIX) * m_boneManager.m_boneInfos.size());
    HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &boneBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_boneMatricesBuffer));
    if (FAILED(hr)) {
        printf("ボーンバッファの生成に失敗しました。\n");
    }

    //頂点数とシェイプキー数を保持
    Contents contents{};
    contents.vertexCount = humanoidMesh.vertexCount;
    contents.shapeCount = static_cast<UINT>(humanoidMesh.shapeWeights.size());
    void* pContentsBuffer;
    pMesh->contentsBuffer->Map(0, nullptr, &pContentsBuffer);
    if (pContentsBuffer)
        memcpy(pContentsBuffer, &contents, sizeof(Contents));
    pMesh->contentsBuffer->Unmap(0, nullptr);
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
        void* pShapeWeightsBuffer;
        humanoidMesh.pMesh->shapeWeightsBuffer->Map(0, nullptr, &pShapeWeightsBuffer);
        memcpy(pShapeWeightsBuffer, humanoidMesh.shapeWeights.data(), shapeWeightsSize);
        humanoidMesh.pMesh->shapeWeightsBuffer->Unmap(0, nullptr);

        //シェイプキーの位置情報を保持するリソースを作成
        CreateShapeDeltasTexture(humanoidMesh);
    }
}

void Character::CreateShapeDeltasTexture(HumanoidMesh& humanoidMesh)
{
    //g_Engine->BeginRender();

    //必要なサイズを計算
    size_t vertexCount = humanoidMesh.vertexCount;
    size_t shapeCount = humanoidMesh.shapeWeights.size();

    UINT maxWidth = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION; //横幅の最大 = 16384
    UINT width = (vertexCount > maxWidth) ? maxWidth : static_cast<UINT>(vertexCount);
    UINT height = static_cast<UINT>(((vertexCount + width - 1) / width) * shapeCount);

    humanoidMesh.pMesh->shapeDeltasBuffer = Texture2D::GetDefaultResource(DXGI_FORMAT_R32G32B32A32_FLOAT, false, width, height);

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

    //アップロード用の一時バッファを作成
    ComPtr<ID3D12Resource> uploadBuffer;
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(rowPitch * height);

    HRESULT hr = m_pDevice->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer)
    );
    if (FAILED(hr)) {
        printf("UploadBufferの作成に失敗しました。\n");
        return;
    }

    //データを書き込む
    void* mappedData;
    uploadBuffer->Map(0, nullptr, &mappedData);
    memcpy(mappedData, textureData.data(), textureData.size() * sizeof(XMFLOAT4));
    uploadBuffer->Unmap(0, nullptr);

    //コマンドリストに転送命令を追加
    D3D12_SUBRESOURCE_DATA textureSubresource = {};
    textureSubresource.pData = textureData.data();
    textureSubresource.RowPitch = rowPitch;
    textureSubresource.SlicePitch = textureSubresource.RowPitch * height;

    UINT64 a = UpdateSubresources(m_pCommandList, humanoidMesh.pMesh->shapeDeltasBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &textureSubresource);

    //リソースステートをシェーダーリソース用に変更
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        humanoidMesh.pMesh->shapeDeltasBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_GENERIC_READ
    );
    m_pCommandList->ResourceBarrier(1, &barrier);

    //コマンドラインを用いたデータの転送は一度レンダーキューを終了させなければならない。
    g_Engine->EndRender();
    g_Engine->BeginRender();

    uploadBuffer.Reset();

    if (humanoidMesh.pMesh->meshName == "Body" && humanoidMesh.pMesh->shapeDeltasBuffer) {
        //printf("a = %llu, bufferSize = %llu, Real = %u\n", a, uploadBufferDesc.Width, width * height);
        //printf("width = %u, rowPitch = %u\n", width, static_cast<UINT>(rowPitch));

        //VerifyShapeDeltasBuffer(humanoidMesh, m_pCommandList);

        /*XMFLOAT4* floatData = reinterpret_cast<XMFLOAT4*>(mappedData);
        for (unsigned int i = 0; i < humanoidMesh.vertexCount; i++)
        {
            printf("ShapeDelta[%u] = (%f, %f, %f)\n", i,
                floatData[i].x, floatData[i].y, floatData[i].z);
        }*/
    }
}

void Character::CalculateBoneTransforms(const aiNode* node, const XMMATRIX& parentTransform)
{
    //ボーンがboneMapに存在するかチェック
    auto it = m_boneManager.m_finalBoneTransforms.find(node->mName.C_Str());
    XMMATRIX nodeTransform = XMMatrixIdentity();

    if (it != m_boneManager.m_finalBoneTransforms.end()) {
        //ボーンのローカル変換行列を取得
        nodeTransform = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&node->mTransformation)));
        //printf("NodeName = %s, x=%f, y=%f, z=%f\n", node->mName.C_Str(), nodeTransform.r[3].m128_f32[0], nodeTransform.r[3].m128_f32[1], nodeTransform.r[3].m128_f32[2]);
    }

    //親ボーンの変換行列との合成（親から子への変換）
    XMMATRIX globalTransform = nodeTransform * parentTransform;

    //ボーンのワールド空間での変換行列を保存（Sphereを置くための位置として使用）
    if (it != m_boneManager.m_finalBoneTransforms.end()) {
        it->second = globalTransform;
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

            //原点から見て、ボーンが存在する位置(オフセット)を取得
            XMMATRIX boneOffset = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<XMFLOAT4X4*>(&bone->mOffsetMatrix)));

            m_boneManager.m_finalBoneTransforms[boneName] = XMMatrixIdentity();

            //ボーンを作成
            Bone boneChild(boneName, boneOffset, boneIndex);

            //手や足は左右を判定 (一旦手の判定をし、足の場合はLoadBoneFamily内で変更)
            if (boneName[0] == 'L' || boneName.at(boneName.size() - 1) == 'L') {
                boneChild.m_bType = BONETYPE_LEFT_ARM;
            }
            if (boneName[0] == 'R' || boneName.at(boneName.size() - 1) == 'R') {
                boneChild.m_bType = BONETYPE_RIGHT_ARM;
            }

            //目のボーンは左右の区別をしない
            if (boneName.find("Eye") != std::string::npos || boneName.find("eye") != std::string::npos) {
                boneChild.m_bType = BONETYPE_DEFAULT;
            }

            //配列に追加
            m_boneManager.m_boneInfos.push_back(XMMatrixIdentity());
            m_boneManager.m_bones.push_back(boneChild);

            //ボーン名とインデックスを紐づけ
            m_boneManager.m_boneMapping[bone->mName.C_Str()] = boneIndex;
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

        Bone bone(boneName, matrix, boneIndex);
        //ボーンタイプ
        bone.m_bType = (BoneType)boneReader.ReadSByte();

        m_boneManager.m_finalBoneTransforms[boneName] = boneReader.ReadMatrix();

        //ボーン名とIndexを紐づけ
        m_boneManager.m_boneMapping[boneName] = boneIndex;
        //配列に追加
        m_boneManager.m_boneInfos.push_back(XMMatrixIdentity());
        m_boneManager.m_bones.push_back(bone);
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
                    if (m_boneManager.m_bones[childIndex].m_bType == BONETYPE_LEFT_ARM) {
                        m_boneManager.m_bones[childIndex].m_bType = BONETYPE_LEFT_LEG;
                    }
                    else if (m_boneManager.m_bones[childIndex].m_bType == BONETYPE_RIGHT_ARM) {
                        m_boneManager.m_bones[childIndex].m_bType = BONETYPE_RIGHT_LEG;
                    }
                    continue;
                }

                if (m_boneManager.m_bones[boneIndex].GetBoneName().find("shoulder") != std::string::npos) {
                    continue;
                }

                //子ボーンは親ボーンの種類を継承
                if (m_boneManager.m_bones[boneIndex].m_bType != BONETYPE_DEFAULT) {
                    m_boneManager.m_bones[childIndex].m_bType = m_boneManager.m_bones[boneIndex].m_bType;
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

    UINT startTime = timeGetTime();
    //圧縮されているシェイプ情報を解凍
    std::vector<char> humanoidBuffer;
    BinaryDecompress(humanoidBuffer, humanoidBufferOriginalSize, compressedBuffer, humanoidBufferCompressedSize);

    UINT endTime = timeGetTime();

    delete[] compressedBuffer;

    //解凍したバッファを読み込む
    BinaryReader humanoidReader(humanoidBuffer);
    UINT humanoidMeshCount = humanoidReader.ReadUInt32();

    for (UINT i = 0; i < humanoidMeshCount; i++) {
        HumanoidMesh humanoidMesh;
        humanoidMesh.pMesh = new Mesh();

        //メッシュ名
        char* meshNameBuf = humanoidReader.ReadBytes(humanoidReader.ReadByte());
        humanoidMesh.pMesh->meshName = meshNameBuf;
        delete[] meshNameBuf;

        UINT shapeMappingCount = humanoidReader.ReadUInt32();
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
            //printf("ShapeName = %s\n", shapeName.c_str());

            /*if (humanoidMesh.meshName == "Body all") {
                printf("%u - %s\n", j, shapeName.c_str());
            }*/
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
}

bool Character::SetTexture(const Mesh* pMesh, const std::string nameOnly)
{
    //std::string dir = "Resource\\Model\\Milltina\\";
    std::string dir = "Resource\\Model\\";

    std::string texPath = dir + nameOnly;
    //std::string normalPath = dir + "Skin_Normal Map.png";

    //テクスチャを作成
    Texture2D* mainTex = Texture2D::Get(texPath);
    //Texture2D* normalMap = Texture2D::Get(normalPath);
    //printf("nameOnly = %s\n", nameOnly.c_str());
    //マテリアルを作成
    m_pDescriptorHeap->SetMainTexture(mainTex->Resource(), nullptr, m_pShadowMapBuffer, pMesh->shapeDeltasBuffer.Get());
    m_textures.push_back(mainTex);
    //m_textures.push_back(normalMap);
    return mainTex->IsSimpleTex();
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
            void* pData = nullptr;
            HRESULT hr = mesh.pMesh->shapeWeightsBuffer->Map(0, nullptr, &pData);
            if (SUCCEEDED(hr))
            {
                memcpy(pData, mesh.shapeWeights.data(), sizeof(float) * mesh.shapeWeights.size()); //各シェイプキーのウェイトをコピー
                mesh.pMesh->shapeWeightsBuffer->Unmap(0, nullptr);
            }
            mesh.bChangeShapeValue = false;
        }
    }
}

void Character::UpdateAnimation()
{
    //アニメーションが存在しない
    if (m_nowAnimationIndex < 0 || m_animations.size() <= m_nowAnimationIndex) {
        return;
    }

    //キーフレームが存在しない
    if (m_animations[m_nowAnimationIndex].m_frames.size() <= 0) {
        return;
    }

    //アニメーション時間を更新
    m_nowAnimationTime += g_Engine->GetFrameTime() * m_animationSpeed;

    //現在のアニメーション時間のフレームを取得
    AnimationFrame* pFrame = m_animations[m_nowAnimationIndex].GetFrame(m_nowAnimationTime);

    //フレームが存在しなければ処理を終了 (主にアニメーションが読み込まれていない場合)
    if (!pFrame) {
        return;
    }

    m_boneManager.m_armatureBone.m_position = pFrame->armatureAnimation.position;
    m_boneManager.m_armatureBone.m_rotation = pFrame->armatureAnimation.rotation;

    //ボーンアニメーション
    for (UINT i = 0; i < pFrame->boneAnimations.size(); i++) {
        std::string boneName = m_animations[m_nowAnimationIndex].m_boneMapping[i];
        UpdateBonePosition(boneName, pFrame->boneAnimations[i].position);
        UpdateBoneRotation(boneName, pFrame->boneAnimations[i].rotation);
    }

    //シェイプキーのアニメーション
    for (UINT i = 0; i < m_animations[m_nowAnimationIndex].m_shapeNames.size(); i++) {
        std::string& shapeName = m_animations[m_nowAnimationIndex].m_shapeNames[i];
        SetShapeWeight(shapeName, pFrame->shapeAnimations[i]);
    }

    //再生中のフレームが最後のフレームだった場合最初に戻す
    if (m_animations[m_nowAnimationIndex].IsLastFrame(pFrame)) {
        m_nowAnimationTime = 0.0f;
    }
}

Character::HumanoidMesh::HumanoidMesh()
    : pMesh(nullptr)
    , vertexCount(0)
    , bChangeShapeValue(true)
{
}
