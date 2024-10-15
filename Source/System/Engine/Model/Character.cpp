#include "Character.h"

Character::Character(const std::string fbxFile, const Camera* pCamera)
	: Model(pCamera)
    , m_animationSpeed(0.77f)
    , m_nowAnimationTime(0.0f)
{
	LoadFBX(fbxFile);

	m_pRootSignature = new RootSignature(m_pDevice, ShaderKinds::BoneShader);
	m_pPipelineState = new PipelineState(m_pDevice, m_pRootSignature);

	printf("モデル:%sをロードしました。\n", fbxFile.c_str());
}

void Character::LoadAnimation(std::string animFile)
{
    m_pAnimation = g_Engine->GetAnimation(animFile);
}

void Character::Update()
{
    //ボーンのマトリックスを更新
    UpdateBoneTransform();

    //シェイプキーのウェイトを更新
    UpdateShapeKeys();

    //アニメーションの更新
    UpdateAnimation();

    Model::Update();
}

void Character::LoadFBX(const std::string& fbxFile)
{
    Assimp::Importer importer;

    //モデル読み込み時のフラグ。メッシュのポリゴンはすべて三角形にし、ボーンが存在する場合は影響を受けるウェイトを4つまでにする
    UINT flag = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
        aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights;// | aiProcess_MakeLeftHanded;
    const aiScene* scene = importer.ReadFile(fbxFile, flag);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        printf("FBXファイルをロードできませんでした。\n");
        return;
    }

    ProcessNode(scene, scene->mRootNode);

    //ボーンに親子関係を付ける
    LoadBoneFamily(scene->mRootNode);

    //マテリアルの読み込み
    m_pDescriptorHeap = new DescriptorHeap();

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
                DescriptorHandle* handle = m_pDescriptorHeap->Register(mainTex);

                g_materials.push_back(handle);
                //マテリアルインデックスは最後に追加したものを使用
                m_meshes[i]->materialIndex = (BYTE)(g_materials.size() - 1);
            }
            else {
                m_meshes[i]->materialIndex = (BYTE)index;
                g_materials[index]->UseCount++;
            }
        }
    }
}

void Character::ProcessNode(const aiScene* scene, aiNode* node) {
    //メッシュを処理
    for (UINT i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        HumanoidMesh humanoidMesh{};
        Mesh* pMesh = ProcessMesh(scene, mesh, humanoidMesh);
        m_meshes.push_back(pMesh);
        m_humanoidMeshes.push_back(humanoidMesh);
    }

    // 子ノードも再帰的に処理
    for (UINT i = 0; i < node->mNumChildren; i++) {
        ProcessNode(scene, node->mChildren[i]);
    }
}

Model::Mesh* Character::ProcessMesh(const aiScene* scene, aiMesh* mesh, HumanoidMesh& humanoidMesh) {
    std::vector<Vertex> vertices;
    std::vector<UINT> indices;

    //頂点の処理
    for (UINT i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex{};

        vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
        vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
        vertex.color = { 1.0f, 1.0f,1.0f,1.0f };
        if (mesh->mTextureCoords[0]) {
            vertex.texCoords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        }
        else {
            vertex.texCoords = { 0.0f, 0.0f };
        }

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

    //頂点バッファの作成
    Mesh* meshData = new Mesh();
    humanoidMesh.pMesh = meshData;

    //ボーンの処理
    LoadBones(scene, mesh, vertices);

    //シェイプキーの処理
    LoadShapeKey(mesh, vertices, humanoidMesh);

    //シェーダーに必要なバッファを生成
    CreateBuffer(meshData, vertices, indices, humanoidMesh);

    return meshData;
}

void Character::CreateBuffer(Mesh* pMesh, std::vector<Vertex>& vertices, std::vector<UINT>& indices, HumanoidMesh& humanoidMesh)
{
    //頂点バッファを設定
    const UINT vertexBufferSize = static_cast<UINT>(sizeof(Vertex) * vertices.size());

    //ヒープ設定
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    //頂点バッファのリソース
    CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    //デバイスで作成
    HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pMesh->vertexBuffer));

    if (FAILED(hr)) {
        printf("頂点バッファの生成に失敗しました。%1xl\n", hr);
    }

    //頂点データをGPUに送信
    void* vertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    pMesh->vertexBuffer->Map(0, &readRange, &vertexDataBegin);
    memcpy(vertexDataBegin, vertices.data(), vertexBufferSize);
    pMesh->vertexBuffer->Unmap(0, nullptr);

    pMesh->vertexBufferView.BufferLocation = pMesh->vertexBuffer->GetGPUVirtualAddress();
    pMesh->vertexBufferView.StrideInBytes = sizeof(Vertex);
    pMesh->vertexBufferView.SizeInBytes = vertexBufferSize;

    //インデックスバッファの作成
    const UINT indexBufferSize = static_cast<UINT>(sizeof(UINT) * indices.size());

    CD3DX12_RESOURCE_DESC indexBuffer = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

    hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &indexBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pMesh->indexBuffer));
    if (FAILED(hr)) {
        printf("インデックスバッファの生成に失敗しました。\n");
    }

    //インデックスデータをGPUメモリに記録
    void* indexDataBegin;
    pMesh->indexBuffer->Map(0, &readRange, &indexDataBegin);
    memcpy(indexDataBegin, indices.data(), indexBufferSize);
    pMesh->indexBuffer->Unmap(0, nullptr);

    pMesh->indexBufferView.BufferLocation = pMesh->indexBuffer->GetGPUVirtualAddress();
    pMesh->indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    pMesh->indexBufferView.SizeInBytes = indexBufferSize;

    pMesh->indexCount = static_cast<UINT>(indices.size());

    //ボーン情報のリソースを作成
    CD3DX12_RESOURCE_DESC boneBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(XMMATRIX) * m_boneInfos.size());
    hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &boneBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_boneMatricesBuffer));
    if (FAILED(hr)) {
        printf("ボーンバッファの生成に失敗しました。\n");
    }

    //頂点数を保持する用のリソースを作成
    CD3DX12_RESOURCE_DESC contentsBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Contents));
    hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &contentsBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pMesh->contentsBuffer));
    if (FAILED(hr)) {
        printf("コンテンツバッファの生成に失敗しました。\n");
    }

    Contents contents{};
    contents.vertexCount = static_cast<UINT>(vertices.size());
    contents.shapeCount = static_cast<UINT>(humanoidMesh.shapeWeights.size());
    void* pContentsBuffer;
    pMesh->contentsBuffer->Map(0, nullptr, &pContentsBuffer);
    memcpy(pContentsBuffer, &contents, sizeof(Contents));
    pMesh->contentsBuffer->Unmap(0, nullptr);

    //シェイプキーの情報を保持する用のリソースを作成
    if (humanoidMesh.shapeWeights.size() > 0) {
        //シェイプキーのウェイト情報を保持するリソースを作成
        UINT64 shapeWeightsSize = sizeof(float) * humanoidMesh.shapeWeights.size();
        D3D12_RESOURCE_DESC shapeWeightsBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(shapeWeightsSize);
        hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &shapeWeightsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pMesh->shapeWeightsBuffer));
        if (FAILED(hr)) {
            printf("シェイプキーウェイトバッファの生成に失敗しました。%ld\n", hr);
        }

        //ウェイト情報の初期値を入れる (Update関数で常に更新される)
        void* pShapeWeightsBuffer;
        pMesh->shapeWeightsBuffer->Map(0, nullptr, &pShapeWeightsBuffer);
        memcpy(pShapeWeightsBuffer, humanoidMesh.shapeWeights.data(), shapeWeightsSize);
        pMesh->shapeWeightsBuffer->Unmap(0, nullptr);

        //シェイプキーの位置情報を保持するリソースを作成
        /*UINT64 shapeDeltaBufferSize = sizeof(XMFLOAT3) * humanoidMesh.shapeWeights.size() * humanoidMesh.vertexCount;
        D3D12_RESOURCE_DESC shapeDeltasBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(shapeDeltaBufferSize);
        hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &shapeDeltasBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pMesh->shapeDeltasBuffer));

        void* pShapeDeltaBuffer;
        pMesh->shapeDeltasBuffer->Map(0, nullptr, &pShapeDeltaBuffer);
        memcpy(pShapeDeltaBuffer, humanoidMesh.shapeDeltas.data(), shapeDeltaBufferSize);
        pMesh->shapeDeltasBuffer->Unmap(0, nullptr);*/
        //printf("--------------------------%zu-----------------------\n", humanoidMesh.shapeDeltas.size());
        CreateShapeDeltasTexture(humanoidMesh);
    }
}

void Character::CreateShapeDeltasTexture(HumanoidMesh& humanoidMesh)
{
    //GPUバッファ（StructuredBuffer）のリソースを作成
    size_t bufferSize = sizeof(XMFLOAT3) * humanoidMesh.shapeWeights.size() * humanoidMesh.vertexCount; //必要なバッファサイズを計算
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    HRESULT hr = m_pDevice->CreateCommittedResource(&defaultHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, 
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&humanoidMesh.pMesh->shapeDeltasBuffer));
    if (FAILED(hr)) {
        printf("shapeDeltasBufferの作成に失敗しました。\n");
    }

    //一時アップロードバッファを作成
    ComPtr<ID3D12Resource> uploadBuffer;
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    hr = m_pDevice->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
    if (FAILED(hr) || !uploadBuffer) {
        printf("uploadBufferの作成に失敗しました。\n");
    }

    //一時バッファにデータをコピー
    void* mappedData;
    uploadBuffer->Map(0, nullptr, &mappedData);
    memcpy(mappedData, humanoidMesh.shapeDeltas.data(), bufferSize);
    uploadBuffer->Unmap(0, nullptr);

    //コマンドリストにコピーコマンドを送る (アップロードバッファをshapeDeltasBufferにコピー)
    g_Engine->CommandList()->CopyResource(humanoidMesh.pMesh->shapeDeltasBuffer.Get(), uploadBuffer.Get());

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        humanoidMesh.pMesh->shapeDeltasBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
    );
    g_Engine->CommandList()->ResourceBarrier(1, &barrier);

    //コマンドラインを用いたデータの転送は一度レンダーキューを終了させなければならない。 (そのため読み込む際、数フレーム描画に遅延が生じる可能性あり)
    g_Engine->EndRender();
    g_Engine->BeginRender();

    //一度設定したら変更しないためクリア
    humanoidMesh.shapeDeltas.clear();
}

static XMVECTOR ExtractEulerAngles(const XMMATRIX& matrix) {
    float pitch, yaw, roll;

    // Y軸方向の回転（yaw）
    yaw = asinf(-matrix.r[2].m128_f32[0]);

    if (cosf(yaw) > 0.0001f) {
        // X軸とZ軸方向の回転（pitch と roll）
        pitch = atan2f(matrix.r[2].m128_f32[1], matrix.r[2].m128_f32[2]);
        roll = atan2f(matrix.r[1].m128_f32[0], matrix.r[0].m128_f32[0]);
    }
    else {
        // Y軸が 90度または -90度の時は、Gimbal Lockに対応
        pitch = atan2f(-matrix.r[0].m128_f32[2], matrix.r[1].m128_f32[1]);
        roll = 0.0f;
    }

    return XMVectorSet(pitch, yaw, roll, 0.0f); // ラジアン角で出力
}

void Character::LoadBones(const aiScene* scene, aiMesh* mesh, std::vector<Vertex>& vertices)
{
    for (UINT i = 0; i < mesh->mNumBones; i++) {
        aiBone* bone = mesh->mBones[i];
        UINT boneIndex = 0;

        // ボーンがまだ登録されていない場合、マッピングを追加
        if (m_boneMapping.find(bone->mName.C_Str()) == m_boneMapping.end()) {
            //追加するボーンのインデックスを取得
            boneIndex = static_cast<UINT>(m_boneInfos.size());

            //原点から見て、ボーンが存在する位置(オフセット)を取得
            XMMATRIX boneOffset = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<XMFLOAT4X4*>(&bone->mOffsetMatrix)));

            //ボーンを作成
            Bone boneChild(bone->mName.C_Str(), boneOffset);

            XMMATRIX& m = boneChild.GetBoneOffset();

            XMVECTOR eulerAngles = ExtractEulerAngles(m);
            // 各軸の回転角（ラジアン）
            float pitch = XMVectorGetX(eulerAngles); // X軸の回転（ピッチ）
            float yaw = XMVectorGetY(eulerAngles);   // Y軸の回転（ヨー）
            float roll = XMVectorGetZ(eulerAngles);  // Z軸の回転（ロール）

            if (abs(yaw) < 1.0) {
                //Y軸とZ軸を交換  (デフォルトはZ軸が高さのため)
                float boneOffsetZ = m.r[3].m128_f32[2];
                m.r[3].m128_f32[2] = m.r[3].m128_f32[1];
                m.r[3].m128_f32[1] = boneOffsetZ;
                boneChild.m_bFlipRot = true;
            }
            else {
                if (m.r[3].m128_f32[0] < 0.0f) {
                    m.r[3].m128_f32[1] = -m.r[3].m128_f32[1];
                    m.r[3].m128_f32[0] = -m.r[3].m128_f32[0];
                }
                float boneOffsetY = m.r[3].m128_f32[1];
                m.r[3].m128_f32[1] = m.r[3].m128_f32[0];
                m.r[3].m128_f32[0] = boneOffsetY;
            }

            //なぜかオフセットのプラスマイナスが反転してしまうボーンがあるため、高さ(Y軸)のみ常にプラスへ修正
            if (m.r[3].m128_f32[2] < 0.0f) {
                m.r[3].m128_f32[2] = -m.r[3].m128_f32[2];
            }
            if (m.r[3].m128_f32[1] > 0.0f) {
                m.r[3].m128_f32[1] = -m.r[3].m128_f32[1];
            }

            //float boneOffsetZ = boneOffset.r[3].m128_f32[1];
            //boneOffset.r[3].m128_f32[1] = boneOffset.r[3].m128_f32[0];
            //boneOffset.r[3].m128_f32[0] = boneOffsetZ;

            /*if (boneChild.GetBoneName() == "Left arm") {
                printf("Left arm = \n");
                for (int i = 0; i < 4; i++) {
                    printf("%f, %f, %f, %f\n", m.r[i].m128_f32[0], m.r[i].m128_f32[1], m.r[i].m128_f32[2], m.r[i].m128_f32[3]);
                }
                printf("Rotation = {%f, %f, %f}\n", pitch, yaw, roll);
            }
            if (boneChild.GetBoneName() == "Right arm") {
                printf("Right arm = \n");
                for (int i = 0; i < 4; i++) {
                    printf("%f, %f, %f, %f\n", m.r[i].m128_f32[0], m.r[i].m128_f32[1], m.r[i].m128_f32[2], m.r[i].m128_f32[3]);
                }
                printf("Rotation = {%f, %f, %f}\n", pitch, yaw, roll);
            }
            if (boneChild.GetBoneName() == "Left elbow") {
                printf("Left elbow = \n");
                for (int i = 0; i < 4; i++) {
                    printf("%f, %f, %f, %f\n", m.r[i].m128_f32[0], m.r[i].m128_f32[1], m.r[i].m128_f32[2], m.r[i].m128_f32[3]);
                }
                printf("Rotation = {%f, %f, %f}\n", pitch, yaw, roll);
            }
            if (boneChild.GetBoneName() == "Right elbow") {
                printf("Right elbow = \n");
                for (int i = 0; i < 4; i++) {
                    printf("%f, %f, %f, %f\n", m.r[i].m128_f32[0], m.r[i].m128_f32[1], m.r[i].m128_f32[2], m.r[i].m128_f32[3]);
                }
                printf("Rotation = {%f, %f, %f}\n", pitch, yaw, roll);
            }
            if (boneChild.GetBoneName() == "Left leg") {
                printf("Left leg = \n");
                for (int i = 0; i < 4; i++) {
                    printf("%f, %f, %f, %f\n", m.r[i].m128_f32[0], m.r[i].m128_f32[1], m.r[i].m128_f32[2], m.r[i].m128_f32[3]);
                }
                printf("Rotation = {%f, %f, %f}\n", pitch, yaw, roll);
            }
            if (boneChild.GetBoneName() == "Right leg") {
                printf("Right leg = \n");
                for (int i = 0; i < 4; i++) {
                    printf("%f, %f, %f, %f\n", m.r[i].m128_f32[0], m.r[i].m128_f32[1], m.r[i].m128_f32[2], m.r[i].m128_f32[3]);
                }
                printf("Rotation = {%f, %f, %f}\n", pitch, yaw, roll);
            }
            if (boneChild.GetBoneName() == "Left knee") {
                printf("Left knee = \n");
                for (int i = 0; i < 4; i++) {
                    printf("%f, %f, %f, %f\n", m.r[i].m128_f32[0], m.r[i].m128_f32[1], m.r[i].m128_f32[2], m.r[i].m128_f32[3]);
                }
                printf("Rotation = {%f, %f, %f}\n", pitch, yaw, roll);
            }
            if (boneChild.GetBoneName() == "Right knee") {
                printf("Right knee = \n");
                for (int i = 0; i < 4; i++) {
                    printf("%f, %f, %f, %f\n", m.r[i].m128_f32[0], m.r[i].m128_f32[1], m.r[i].m128_f32[2], m.r[i].m128_f32[3]);
                }
                printf("Rotation = {%f, %f, %f}\n", pitch, yaw, roll);
            }*/

            //配列に追加
            m_boneInfos.push_back(XMMatrixIdentity());
            m_bones.push_back(boneChild);

            //ボーン名とインデックスを紐づけ
            m_boneMapping[bone->mName.C_Str()] = boneIndex;
        }
        else {
            //同名のボーンが存在する場合それを使用 (異なるメッシュ間で同名のボーンの場合、ほとんどが共有されているため)
            boneIndex = m_boneMapping[bone->mName.C_Str()];
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
                // 4つ以上のボーンウェイトはサポート外
                //printf("頂点 %d は 4 つ以上のボーンウェイトを持っています。\n", vertexID);
            }
        }
    }
}

//ボーンの親子関係を取得
void Character::LoadBoneFamily(const aiNode* node)
{
    if (m_boneMapping.find(node->mName.C_Str()) != m_boneMapping.end()) {
        UINT boneIndex = m_boneMapping[node->mName.C_Str()];
        for (UINT i = 0; i < node->mNumChildren; i++) {
            if (m_boneMapping.find(node->mChildren[i]->mName.C_Str()) != m_boneMapping.end()) {
                UINT childIndex = m_boneMapping[node->mChildren[i]->mName.C_Str()];

                //子ボーンと親ボーンを設定
                m_bones[boneIndex].AddChildBone(&m_bones[childIndex], childIndex);
                m_bones[childIndex].SetParentBone(boneIndex);
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
        //
        if (humanoidMesh.shapeMapping.find(shapeName) == humanoidMesh.shapeMapping.end()) {
            UINT shapeIndex = static_cast<UINT>(humanoidMesh.shapeWeights.size());

            humanoidMesh.shapeWeights.push_back(0.0f);
            humanoidMesh.shapeMapping[shapeName] = shapeIndex;

            printf("shapeName = %s, Index = %d\n", shapeName.c_str(), shapeIndex);

            for (UINT j = 0; j < animMesh->mNumVertices; j++) {
                aiVector3D& vec = animMesh->mVertices[j];
                //シェイプキーの、100%のときのその頂点の位置
                XMFLOAT3 shapePos = XMFLOAT3(vec.x, vec.y, vec.z);
                //元の位置を引く
                shapePos.x -= vertices[j].position.x;
                shapePos.y -= vertices[j].position.y;
                shapePos.z -= vertices[j].position.z;
                humanoidMesh.shapeDeltas.push_back(shapePos);
                //printf("Shape = %d\n", shapeIndex);
            }
        }
    }
}

//ボーンの位置を更新
void Character::UpdateBonePosition(std::string boneName, XMFLOAT3& position)
{
    if (m_boneMapping.find(boneName) == m_boneMapping.end()) {
        return;
    }

    UINT boneIndex = m_boneMapping[boneName];

    m_bones[boneIndex].m_position = position;
}

//ボーンの回転を変更
void Character::UpdateBoneRotation(std::string boneName, XMFLOAT4& rotation)
{
    if (m_boneMapping.find(boneName) == m_boneMapping.end()) {
        return;
    }

    UINT boneIndex = m_boneMapping[boneName];

    m_bones[boneIndex].m_rotation = rotation;
}

//ボーンのスケールを変更
void Character::UpdateBoneScale(std::string boneName, XMFLOAT3& scale)
{
    if (m_boneMapping.find(boneName) == m_boneMapping.end()) {
        return;
    }

    UINT boneIndex = m_boneMapping[boneName];

    m_bones[boneIndex].m_scale = scale;
}

//シェイプキーのウェイトを更新
void Character::SetShapeWeight(const std::string shapeName, float weight)
{
    if (weight < 0.0f)
        weight = 0.0f;
    else if (weight > 1.0f)
        weight = 1.0f;

    for (HumanoidMesh& mesh : m_humanoidMeshes) {
        if (mesh.shapeMapping.find(shapeName) == mesh.shapeMapping.end()) {
            continue;
        }

        UINT shapeIndex = mesh.shapeMapping[shapeName];
        //printf("ShapeIndex = %d\n", shapeIndex);
        mesh.shapeWeights[shapeIndex] = weight;
        //for (UINT i = 0; i < shapeWeights.size(); i++) {
            //SetShapeWeight(i, weight);
        //}
    }
}

//すべてのボーン名を取得
std::vector<std::string> Character::GetBoneNames()
{
    std::vector<std::string> boneNames;
    for (Bone& bone : m_bones) {
        boneNames.push_back(bone.GetBoneName());
    }
    return boneNames;
}

void Character::UpdateBoneTransform(UINT boneIndex, XMMATRIX& parentMatrix)
{
    XMMATRIX parentTransform = XMMatrixIdentity();

    //親ボーンが存在する場合、そのワールド座標を適用
    if (m_bones[boneIndex].GetParentBoneIndex() != UINT32_MAX) {
        parentTransform = m_boneInfos[m_bones[boneIndex].GetParentBoneIndex()];
    }

    //ボーンの最終的な位置、回転、スケールを決定

    Bone& bone = m_bones[boneIndex];
    XMMATRIX scale = XMMatrixScaling(bone.m_scale.x, bone.m_scale.y, bone.m_scale.z);

    //XMMATRIX rotX = XMMatrixRotationX(XMConvertToRadians(bone.m_rotation.x));  //X軸
    //XMMATRIX rotY = XMMatrixRotationY(XMConvertToRadians(bone.m_rotation.z));  //Unity基準にするため、Y軸とZ軸を入れ替え
    //XMMATRIX rotZ = XMMatrixRotationZ(XMConvertToRadians(bone.m_rotation.y));  //Unity基準にするため、Y軸とZ軸を入れ替え
    //XMMATRIX rot = rotX * rotY * rotZ;
    //XMVECTOR rotVec = XMQuaternionRotationRollPitchYaw(bone.m_rotation.x, bone.m_rotation.z, bone.m_rotation.y);
    XMVECTOR rotVec;
    //rotVec = XMVectorSet(bone.m_rotation.x, bone.m_rotation.z, bone.m_rotation.y, bone.m_rotation.w);
    if (bone.m_bFlipRot) {
        rotVec = XMVectorSet(-bone.m_rotation.x, bone.m_rotation.z, bone.m_rotation.y, bone.m_rotation.w);
    }
    else {
        rotVec = XMVectorSet(bone.m_rotation.x, bone.m_rotation.z, bone.m_rotation.y, bone.m_rotation.w);
    }
    XMMATRIX rot = XMMatrixRotationQuaternion(rotVec);

    XMMATRIX pos = XMMatrixTranslation(bone.m_position.x, bone.m_position.y, bone.m_position.z);
    //XMMATRIX offsetBack = XMMatrixTranslation(bone.GetBoneOffset().r[3].m128_f32[0], bone.GetBoneOffset().r[3].m128_f32[1], bone.GetBoneOffset().r[3].m128_f32[2]);
    //XMMATRIX offsetOrigin = XMMatrixTranslation(-bone.GetBoneOffset().r[3].m128_f32[0], -bone.GetBoneOffset().r[3].m128_f32[1], -bone.GetBoneOffset().r[3].m128_f32[2]);
    XMMATRIX offsetBack = XMMatrixInverse(nullptr, bone.GetBoneOffset());

    XMMATRIX boneTransform = scale * offsetBack * rot * bone.GetBoneOffset() * pos;

    //親のワールド変換とローカル変換を合成
    XMMATRIX finalTransform = parentTransform * XMMatrixTranspose(boneTransform);

    //シェーダーに送るワールド座標に代入
    m_boneInfos[boneIndex] = finalTransform;

    // 子ボーンにも変換を伝播
    for (UINT i = 0; i < bone.GetChildBoneCount(); i++) {
        UpdateBoneTransform(bone.GetChildBoneIndex(i), m_boneInfos[boneIndex]);
    }
}

void Character::UpdateBoneTransform()
{
    //ボーンの最終位置を決定
    XMMATRIX m = XMMatrixIdentity();
    UpdateBoneTransform(0, m);

    //ボーンバッファに送信
    void* pData;
    HRESULT hr = m_boneMatricesBuffer->Map(0, nullptr, &pData);
    if (SUCCEEDED(hr))
    {
        memcpy(pData, m_boneInfos.data(), sizeof(XMMATRIX) * m_boneInfos.size()); // ボーンマトリックスをコピー
        m_boneMatricesBuffer->Unmap(0, nullptr);
    }
}

void Character::UpdateShapeKeys()
{
    for (HumanoidMesh& mesh : m_humanoidMeshes) {
        //シェイプバッファに送信
        if (mesh.pMesh->shapeWeightsBuffer) {
            void* pData = nullptr;
            HRESULT hr = mesh.pMesh->shapeWeightsBuffer->Map(0, nullptr, &pData);
            if (SUCCEEDED(hr))
            {
                memcpy(pData, mesh.shapeWeights.data(), sizeof(float) * mesh.shapeWeights.size()); // 各シェイプキーのウェイトをコピー
                mesh.pMesh->shapeWeightsBuffer->Unmap(0, nullptr);
            }
        }
    }
}

void Character::UpdateAnimation()
{
    if (!m_pAnimation) {
        return;
    }

    m_nowAnimationTime += g_Engine->GetFrameTime() * m_animationSpeed;

    AnimationFrame* pFrame = m_pAnimation->GetFrame(m_nowAnimationTime);

    for (UINT i = 0; i < pFrame->animations.size(); i++) {
        std::string boneName = m_pAnimation->boneMapping[i];
        if (boneName[0] == 'T' && boneName[1] == 'h' && boneName[2] == 'u' && boneName[3] == 'm' && boneName[4] == 'b') {

        }
        else {
            UpdateBonePosition(boneName, pFrame->animations[i].position);
            UpdateBoneRotation(boneName, pFrame->animations[i].rotation);
        }
    }

    //再生中のフレームが最後のフレームだった場合最初に戻す
    if (m_pAnimation->IsLastFrame(pFrame)) {
        m_nowAnimationTime = 0.0f;
    }
}
