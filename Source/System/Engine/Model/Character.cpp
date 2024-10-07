#include "Character.h"

Character::Character(const std::string fbxFile, const Camera* pCamera)
	: Model(pCamera)
{
	LoadFBX(fbxFile);

	m_pRootSignature = new RootSignature(m_pDevice, ShaderKinds::BoneShader);
	m_pPipelineState = new PipelineState(m_pDevice, m_pRootSignature);

	printf("モデル:%sをロードしました。\n", fbxFile.c_str());
}

void Character::Update()
{
    //ボーンのマトリックスを更新
    UpdateBoneTransform();

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
                meshes[i].materialIndex = (BYTE)(g_materials.size() - 1);
            }
            else {
                meshes[i].materialIndex = (BYTE)index;
                g_materials[index]->UseCount++;
            }
        }
    }
}

void Character::ProcessNode(const aiScene* scene, aiNode* node) {
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

Model::Mesh Character::ProcessMesh(const aiScene* scene, aiMesh* mesh) {
    std::vector<Vertex> vertices;
    std::vector<UINT> indices;

    //頂点の処理
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

    //インデックスの処理
    for (UINT i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (UINT j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    //頂点バッファの作成
    Mesh meshData;

    //ボーンの処理
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
    HRESULT result = m_pDevice->CreateCommittedResource(
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

    result = m_pDevice->CreateCommittedResource(
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

    //インデックスデータをGPUメモリに記録
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

    HRESULT hr = m_pDevice->CreateCommittedResource(
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

void Character::LoadBones(const aiScene* scene, Mesh& meshStruct, aiMesh* mesh, std::vector<Vertex>& vertices)
{
    for (UINT i = 0; i < mesh->mNumBones; i++) {
        aiBone* bone = mesh->mBones[i];
        UINT boneIndex = 0;

        // ボーンがまだ登録されていない場合、マッピングを追加
        if (boneMapping.find(bone->mName.C_Str()) == boneMapping.end()) {
            //追加するボーンのインデックスを取得
            boneIndex = static_cast<UINT>(boneInfos.size());

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
            boneInfos.push_back(XMMatrixIdentity());
            bones.push_back(boneChild);

            //ボーン名とインデックスを紐づけ
            boneMapping[bone->mName.C_Str()] = boneIndex;
        }
        else {
            //同名のボーンが存在する場合それを使用 (異なるメッシュ間で同名のボーンの場合、ほとんどが共有されているため)
            boneIndex = boneMapping[bone->mName.C_Str()];
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
}

//ボーンの親子関係を取得
void Character::LoadBoneFamily(const aiNode* node)
{
    if (boneMapping.find(node->mName.C_Str()) != boneMapping.end()) {
        UINT boneIndex = boneMapping[node->mName.C_Str()];
        for (UINT i = 0; i < node->mNumChildren; i++) {
            if (boneMapping.find(node->mChildren[i]->mName.C_Str()) != boneMapping.end()) {
                UINT childIndex = boneMapping[node->mChildren[i]->mName.C_Str()];

                //子ボーンと親ボーンを設定
                bones[boneIndex].AddChildBone(&bones[childIndex], childIndex);
                bones[childIndex].SetParentBone(boneIndex);
            }
        }
    }

    //子ボーンも同様に設定
    for (UINT i = 0; i < node->mNumChildren; i++) {
        LoadBoneFamily(node->mChildren[i]);
    }
}

void Character::UpdateBonePosition(std::string boneName, XMFLOAT3& position)
{
    if (boneMapping.find(boneName) == boneMapping.end()) {
        return;
    }

    UINT boneIndex = boneMapping[boneName];

    bones[boneIndex].m_position = position;
}
void Character::UpdateBoneRotation(std::string boneName, XMFLOAT4& rotation)
{
    if (boneMapping.find(boneName) == boneMapping.end()) {
        return;
    }

    UINT boneIndex = boneMapping[boneName];

    bones[boneIndex].m_rotation = rotation;
}
void Character::UpdateBoneScale(std::string boneName, XMFLOAT3& scale)
{
    if (boneMapping.find(boneName) == boneMapping.end()) {
        return;
    }

    UINT boneIndex = boneMapping[boneName];

    bones[boneIndex].m_scale = scale;
}

std::vector<std::string> Character::GetBoneNames()
{
    std::vector<std::string> boneNames;
    for (Bone& bone : bones) {
        boneNames.push_back(bone.GetBoneName());
    }
    return boneNames;
}

void Character::UpdateBoneTransform(UINT boneIndex, XMMATRIX& parentMatrix)
{
    XMMATRIX parentTransform = XMMatrixIdentity();

    //親ボーンが存在する場合、そのワールド座標を適用
    if (bones[boneIndex].GetParentBoneIndex() != UINT32_MAX) {
        parentTransform = boneInfos[bones[boneIndex].GetParentBoneIndex()];
    }

    //ボーンの最終的な位置、回転、スケールを決定

    Bone& bone = bones[boneIndex];
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
    if (bone.GetBoneName() == "Left elbow") {
        printf("x = %f, y = %f, z = %f, w = %f\n", rotVec.m128_f32[0], rotVec.m128_f32[1], rotVec.m128_f32[2], rotVec.m128_f32[3]);
    }

    XMMATRIX pos = XMMatrixTranslation(bone.m_position.x, bone.m_position.y, bone.m_position.z);
    //XMMATRIX offsetBack = XMMatrixTranslation(bone.GetBoneOffset().r[3].m128_f32[0], bone.GetBoneOffset().r[3].m128_f32[1], bone.GetBoneOffset().r[3].m128_f32[2]);
    //XMMATRIX offsetOrigin = XMMatrixTranslation(-bone.GetBoneOffset().r[3].m128_f32[0], -bone.GetBoneOffset().r[3].m128_f32[1], -bone.GetBoneOffset().r[3].m128_f32[2]);
    XMMATRIX offsetBack = XMMatrixInverse(nullptr, bone.GetBoneOffset());

    XMMATRIX boneTransform = scale * offsetBack * rot * bone.GetBoneOffset() * pos;

    //親のワールド変換とローカル変換を合成
    XMMATRIX finalTransform = parentTransform * XMMatrixTranspose(boneTransform);

    //シェーダーに送るワールド座標に代入
    boneInfos[boneIndex] = finalTransform;

    // 子ボーンにも変換を伝播
    for (UINT i = 0; i < bone.GetChildBoneCount(); i++) {
        UpdateBoneTransform(bone.GetChildBoneIndex(i), boneInfos[boneIndex]);
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
        memcpy(pData, boneInfos.data(), sizeof(XMMATRIX) * boneInfos.size()); // ボーンマトリックスをコピー
        m_boneMatricesBuffer->Unmap(0, nullptr);
    }
}
