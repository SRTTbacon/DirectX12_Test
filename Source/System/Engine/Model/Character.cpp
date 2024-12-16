#include "Character.h"

Character::Character(const std::string fbxFile, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const Camera* pCamera, DirectionalLight* pDirectionalLight,
    ID3D12Resource* pShadowMapBuffer)
    : Model(pDevice, pCommandList, pCamera, pDirectionalLight, pShadowMapBuffer)
    , m_armatureBone(Bone("Armature", XMMatrixIdentity()))
    , m_animationSpeed(1.0f)
    , m_nowAnimationTime(0.0f)
    , m_nowAnimationIndex(-1)
{
    m_fbxFile = fbxFile;

    CreateConstantBuffer();

    //FBXをロード (今後独自フォーマットに変更予定)
	LoadFBX(fbxFile);

    //ルートシグネチャとパイプラインステートを初期化
	m_pRootSignature = new RootSignature(m_pDevice, ShaderKinds::BoneShader);
	m_pPipelineState = new PipelineState(m_pDevice, m_pRootSignature);

	printf("モデル:%sをロードしました。\n", fbxFile.c_str());
}

UINT Character::AddAnimation(Animation animation)
{
    m_animations.push_back(animation);
    //アニメーションが1
    if (m_animations.size() == 1) {
        m_nowAnimationIndex = 0;
    }
    return (unsigned int)m_animations.size() - 1;
}

void Character::Update(UINT backBufferIndex)
{
    //ボーンのマトリックスを更新
    UpdateBoneTransform();

    //シェイプキーのウェイトを更新
    UpdateShapeKeys();

    //アニメーションの更新
    UpdateAnimation();

    //親クラスの更新
    Model::Update(backBufferIndex);
}

void Character::CalculateBoneTransforms(const aiNode* node, const XMMATRIX& parentTransform)
{
    // ボーンがboneMapに存在するかチェック
    auto it = finalBoneTransforms.find(node->mName.C_Str());
    XMMATRIX nodeTransform = XMMatrixIdentity();

    if (it != finalBoneTransforms.end()) {
        //ボーンのローカル変換行列を取得
        nodeTransform = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&node->mTransformation)));
        //printf("NodeName = %s, x=%f, y=%f, z=%f\n", node->mName.C_Str(), nodeTransform.r[3].m128_f32[0], nodeTransform.r[3].m128_f32[1], nodeTransform.r[3].m128_f32[2]);
    }

    //親ボーンの変換行列との合成（親から子への変換）
    XMMATRIX globalTransform = nodeTransform * parentTransform;

    //ボーンのワールド空間での変換行列を保存（Sphereを置くための位置として使用）
    if (it != finalBoneTransforms.end()) {
        it->second = globalTransform;
    }

    //子ノードに対して再帰的に処理を実行
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        CalculateBoneTransforms(node->mChildren[i], globalTransform);
    }
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

    XMMATRIX mat = XMMatrixIdentity();
    CalculateBoneTransforms(scene->mRootNode, mat);

    //マテリアルの読み込み
    m_pDescriptorHeap = new DescriptorHeap(m_pDevice, scene->mNumMeshes * 2, ShadowSizeHigh);

    std::string dir = "Resource\\Model\\";
    for (size_t i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[i];
        std::string nameOnly = "";
        //メッシュのマテリアルを取得する
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            //ファイル名 = マテリアル名 + .png
            nameOnly = material->GetName().C_Str() + std::string(".png");
            std::string texPath = dir + nameOnly;
            std::wstring wideTexPath = Texture2D::GetWideString(texPath);

            //テクスチャがまだロードされていない場合はロードし、ロード済みの場合は入っているインデックスを参照
            if (textures.find(wideTexPath) == textures.end()) {
                //テクスチャを作成
                Texture2D* mainTex = Texture2D::Get(texPath);
                //マテリアルを作成
                //DescriptorHandle* handle = m_pDescriptorHeap->Register(mainTex);
                m_pDescriptorHeap->SetMainTexture(mainTex->Resource(), m_pShadowMapBuffer);

                //g_materials.push_back(handle);
                //マテリアルインデックスは最後に追加したものを使用
                //m_meshes[i]->materialIndex = (BYTE)(g_materials.size() - 1);
            }
            else {
                //m_meshes[i]->materialIndex = (BYTE)index;
                //g_materials[index]->UseCount++;
                m_pDescriptorHeap->SetMainTexture(textures[wideTexPath]->Resource(), m_pShadowMapBuffer);
            }
        }
        else {
            m_pDescriptorHeap->SetMainTexture(Texture2D::GetWhite()->Resource(), m_pShadowMapBuffer);
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
        CreateShapeDeltasTexture(humanoidMesh);
    }
}

void Character::CreateShapeDeltasTexture(HumanoidMesh& humanoidMesh)
{
    //GPUバッファ（StructuredBuffer）のリソースを作成
    size_t bufferSize = sizeof(XMFLOAT3) * humanoidMesh.shapeWeights.size() * humanoidMesh.vertexCount; //必要なバッファサイズを計算
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    //CPUからの変更を受け付けないシェイプキー用のリソースを作成
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
    m_pCommandList->CopyResource(humanoidMesh.pMesh->shapeDeltasBuffer.Get(), uploadBuffer.Get());

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        humanoidMesh.pMesh->shapeDeltasBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
    );
    m_pCommandList->ResourceBarrier(1, &barrier);

    //コマンドラインを用いたデータの転送は一度レンダーキューを終了させなければならない。 (そのため読み込む際、数フレーム描画に遅延が生じる可能性あり)
    g_Engine->EndRender();
    g_Engine->BeginRender();

    Sleep(5);

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

        std::string boneName = bone->mName.C_Str();

        // ボーンがまだ登録されていない場合、マッピングを追加
        if (m_boneMapping.find(boneName) == m_boneMapping.end()) {
            //追加するボーンのインデックスを取得
            boneIndex = static_cast<UINT>(m_boneInfos.size());

            //原点から見て、ボーンが存在する位置(オフセット)を取得
            XMMATRIX boneOffset = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<XMFLOAT4X4*>(&bone->mOffsetMatrix)));

            finalBoneTransforms[boneName] = XMMatrixIdentity();

            //ボーンを作成
            Bone boneChild(boneName, boneOffset);

            XMMATRIX& m = boneChild.GetBoneOffset();

            XMVECTOR eulerAngles = ExtractEulerAngles(m);
            //各軸の回転角（ラジアン）
            float pitch = XMVectorGetX(eulerAngles); //X軸の回転（ピッチ）
            float yaw = XMVectorGetY(eulerAngles);   //Y軸の回転（ヨー）
            float roll = XMVectorGetZ(eulerAngles);  //Z軸の回転（ロール）

            if (abs(yaw) < 1.0) {
                //Y軸とZ軸を交換  (デフォルトはZ軸が高さのため)
                float boneOffsetZ = m.r[3].m128_f32[2];
                m.r[3].m128_f32[2] = m.r[3].m128_f32[1];
                m.r[3].m128_f32[1] = boneOffsetZ;
                //boneChild.m_bFlipRot = true;
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

            /*if (boneName.find("shoulder") != std::string::npos) {
                if (boneChild.m_bType == BONETYPE_LEFT_ARM) {
                    boneChild.m_bType = BONETYPE_LEFT_SHOULDER;
                }
            }*/
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

                std::string boneName = m_bones[childIndex].GetBoneName();
                bool bExchanged = false;
                if (boneName.find("Leg") != std::string::npos || boneName.find("leg") != std::string::npos) {
                    if (m_bones[childIndex].m_bType == BONETYPE_LEFT_ARM) {
                        m_bones[childIndex].m_bType = BONETYPE_LEFT_LEG;
                    }
                    else if (m_bones[childIndex].m_bType == BONETYPE_RIGHT_ARM) {
                        m_bones[childIndex].m_bType = BONETYPE_RIGHT_LEG;
                    }
                    bExchanged = true;
                }
                if (m_bones[boneIndex].GetBoneName().find("shoulder") != std::string::npos) {
                    bExchanged = true;
                }

                if (!bExchanged && m_bones[boneIndex].m_bType != BONETYPE_DEFAULT) {
                    m_bones[childIndex].m_bType = m_bones[boneIndex].m_bType;
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

void Character::SetAnimationTime(float animTime)
{
    m_nowAnimationTime = animTime;
}

void Character::Test()
{
    //ボーンバッファに送信
    void* pData;
    HRESULT hr = m_boneMatricesBuffer->Map(0, nullptr, &pData);
    if (SUCCEEDED(hr))
    {
        memcpy(pData, m_boneInfos.data(), sizeof(XMMATRIX) * m_boneInfos.size()); // ボーンマトリックスをコピー
        m_boneMatricesBuffer->Unmap(0, nullptr);
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
    else {
        XMMATRIX armatureScale = XMMatrixScaling(m_armatureBone.m_scale.x, m_armatureBone.m_scale.y, m_armatureBone.m_scale.z);
        XMVECTOR armatureRotVec = XMVectorSet(-m_armatureBone.m_rotation.x, -m_armatureBone.m_rotation.z, -m_armatureBone.m_rotation.y, m_armatureBone.m_rotation.w);
        XMMATRIX armatureRot = XMMatrixRotationQuaternion(armatureRotVec);

        XMMATRIX armaturePos = XMMatrixTranslation(-m_armatureBone.m_position.x, -m_armatureBone.m_position.z, m_armatureBone.m_position.y);

        XMMATRIX boneTransform = armatureScale * armatureRot * armaturePos;

        //親のワールド変換とローカル変換を合成
        parentTransform = XMMatrixTranspose(boneTransform);
    }

    //ボーンの最終的な位置、回転、スケールを決定

    Bone& bone = m_bones[boneIndex];
    XMMATRIX scale = XMMatrixScaling(bone.m_scale.x, bone.m_scale.y, bone.m_scale.z);

    //XMMATRIX rotX = XMMatrixRotationX(XMConvertToRadians(bone.m_rotation.x));  //X軸
    //XMMATRIX rotY = XMMatrixRotationY(XMConvertToRadians(bone.m_rotation.z));  //Unity基準にするため、Y軸とZ軸を入れ替え
    //XMMATRIX rotZ = XMMatrixRotationZ(XMConvertToRadians(bone.m_rotation.y));  //Unity基準にするため、Y軸とZ軸を入れ替え
    //XMMATRIX rot = rotX * rotY * rotZ;
    //XMVECTOR rotVec = XMQuaternionRotationRollPitchYaw(bone.m_rotation.x, bone.m_rotation.z, bone.m_rotation.y);
    XMVECTOR rotVec = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    //rotVec = XMVectorSet(bone.m_rotation.x, bone.m_rotation.z, bone.m_rotation.y, bone.m_rotation.w);

    //Unityとの座標系の違いを修正 (導き出すのにめっっっちゃ時間かかった。なんで手と足と胴体で仕様違うの。)
    switch (bone.m_bType)
    {
    case BONETYPE_DEFAULT:
        rotVec = XMVectorSet(-bone.m_rotation.x, -bone.m_rotation.z, bone.m_rotation.y, bone.m_rotation.w);
        break;

    case BONETYPE_LEFT_ARM:
        rotVec = XMVectorSet(bone.m_rotation.y, bone.m_rotation.x, bone.m_rotation.z, bone.m_rotation.w);
        break;

    case BONETYPE_LEFT_LEG:
        rotVec = XMVectorSet(-bone.m_rotation.x, bone.m_rotation.z, -bone.m_rotation.y, bone.m_rotation.w);
        break;

    case BONETYPE_RIGHT_ARM:
        rotVec = XMVectorSet(-bone.m_rotation.y, -bone.m_rotation.x, bone.m_rotation.z, bone.m_rotation.w);
        break;

    case BONETYPE_RIGHT_LEG:
        rotVec = XMVectorSet(bone.m_rotation.x, -bone.m_rotation.z, bone.m_rotation.y, -bone.m_rotation.w);
        break;
    }

    XMMATRIX rot = XMMatrixRotationQuaternion(rotVec);

    XMMATRIX pos = XMMatrixTranslation(bone.m_position.x, bone.m_position.z, bone.m_position.y);
    //XMMATRIX offsetBack = XMMatrixTranslation(bone.GetBoneOffset().r[3].m128_f32[0], bone.GetBoneOffset().r[3].m128_f32[1], bone.GetBoneOffset().r[3].m128_f32[2]);
    //XMMATRIX offsetOrigin = XMMatrixTranslation(-bone.GetBoneOffset().r[3].m128_f32[0], -bone.GetBoneOffset().r[3].m128_f32[1], -bone.GetBoneOffset().r[3].m128_f32[2]);
    //XMMATRIX offsetBack = XMMatrixInverse(nullptr, bone.GetBoneOffset());
    XMMATRIX offsetBack2 = finalBoneTransforms[bone.GetBoneName()];
    XMMATRIX offsetBack = XMMatrixIdentity();
    offsetBack.r[3].m128_f32[0] = offsetBack2.r[3].m128_f32[0];
    offsetBack.r[3].m128_f32[1] = offsetBack2.r[3].m128_f32[1];
    offsetBack.r[3].m128_f32[2] = offsetBack2.r[3].m128_f32[2];

    //XMMATRIX boneTransform = scale * offsetBack * rot * bone.GetBoneOffset() * pos;
    XMMATRIX boneTransform = scale * XMMatrixInverse(nullptr, offsetBack) * rot * offsetBack * pos;

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

    m_armatureBone.m_position = pFrame->armatureAnimation.position;
    m_armatureBone.m_rotation = pFrame->armatureAnimation.rotation;

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
