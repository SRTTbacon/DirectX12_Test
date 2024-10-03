#include "Character.h"

Character::Character(const std::string fbxFile, const Camera* pCamera)
	: Model(pCamera)
{
	LoadFBX(fbxFile);

	m_pRootSignature = new RootSignature(m_pDevice, ShaderKinds::BoneShader);
	m_pPipelineState = new PipelineState(m_pDevice, m_pRootSignature);

	printf("���f��:%s�����[�h���܂����B\n", fbxFile.c_str());
}

void Character::Update()
{
    //�{�[���̃}�g���b�N�X���X�V
    UpdateBoneTransform();

    Model::Update();
}

void Character::LoadFBX(const std::string& fbxFile)
{
    Assimp::Importer importer;

    //���f���ǂݍ��ݎ��̃t���O�B���b�V���̃|���S���͂��ׂĎO�p�`�ɂ��A������W�n�ɕϊ�
    unsigned int flag = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
        aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights;// | aiProcess_MakeLeftHanded;
    const aiScene* scene = importer.ReadFile(fbxFile, flag);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        printf("FBX�t�@�C�������[�h�ł��܂���ł����B\n");
        return;
    }

    ProcessNode(scene, scene->mRootNode);

    //�{�[���ɐe�q�֌W��t����
    LoadBoneFamily(scene->mRootNode);

    XMFLOAT3 rot = XMFLOAT3(0.0f, 0.0f, 0.0f);
    UpdateBoneRotation("Left leg", rot);

    //�}�e���A���̓ǂݍ���
    m_pDescriptorHeap = new DescriptorHeap();

    std::string dir = "Resource\\Model\\";
    for (size_t i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[i];
        std::string nameOnly = "";
        // ���b�V���̃}�e���A�����擾����
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            //�t�@�C���� = �}�e���A���� + .png
            nameOnly = material->GetName().C_Str() + std::string(".png");
            std::string texPath = dir + nameOnly;

            //�e�N�X�`�����܂����[�h����Ă��Ȃ��ꍇ�̓��[�h���A���[�h�ς݂̏ꍇ�͓����Ă���C���f�b�N�X���Q��
            int index = Texture2D::GetTextureIndex(texPath);
            if (index == -1) {
                //�e�N�X�`�����쐬
                Texture2D* mainTex = Texture2D::Get(texPath);
                //�}�e���A�����쐬
                DescriptorHandle* handle = m_pDescriptorHeap->Register(mainTex);

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

void Character::ProcessNode(const aiScene* scene, aiNode* node) {
    // ���b�V��������
    for (UINT i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(ProcessMesh(scene, mesh));
    }

    // �q�m�[�h���ċA�I�ɏ���
    for (UINT i = 0; i < node->mNumChildren; i++) {
        ProcessNode(scene, node->mChildren[i]);
    }
}

Model::Mesh Character::ProcessMesh(const aiScene* scene, aiMesh* mesh) {
    std::vector<Vertex> vertices;
    std::vector<UINT> indices;

    // ���_�̏���
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

    // �C���f�b�N�X�̏���
    for (UINT i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (UINT j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // ���_�o�b�t�@�̍쐬
    Mesh meshData;

    // �{�[���̏���
    LoadBones(scene, meshData, mesh, vertices);

    //���_�o�b�t�@��ݒ�
    const UINT vertexBufferSize = static_cast<UINT>(sizeof(Vertex) * vertices.size());

    //�q�[�v�ݒ�
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    //���_�o�b�t�@�̃��\�[�X
    D3D12_RESOURCE_DESC vertexBufferDesc = {};
    vertexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vertexBufferDesc.Width = vertexBufferSize;
    vertexBufferDesc.Height = 1;
    vertexBufferDesc.DepthOrArraySize = 1;
    vertexBufferDesc.MipLevels = 1;
    vertexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    vertexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    vertexBufferDesc.SampleDesc.Count = 1;

    //�f�o�C�X�ō쐬
    HRESULT result = m_pDevice->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&meshData.vertexBuffer)
    );

    if (FAILED(result)) {
        printf("���_�o�b�t�@�̐����Ɏ��s���܂����B\n");
    }

    //���_�f�[�^��GPU�ɑ��M
    void* vertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    meshData.vertexBuffer->Map(0, &readRange, &vertexDataBegin);
    memcpy(vertexDataBegin, vertices.data(), vertexBufferSize);
    meshData.vertexBuffer->Unmap(0, nullptr);

    meshData.vertexBufferView.BufferLocation = meshData.vertexBuffer->GetGPUVirtualAddress();
    meshData.vertexBufferView.StrideInBytes = sizeof(Vertex);
    meshData.vertexBufferView.SizeInBytes = vertexBufferSize;

    //�C���f�b�N�X�o�b�t�@�̍쐬
    const UINT indexBufferSize = static_cast<UINT>(sizeof(UINT) * indices.size());

    //�C���f�b�N�X�o�b�t�@�̃��\�[�X
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
        printf("�C���f�b�N�X�o�b�t�@�̐����Ɏ��s���܂����B\n");
    }

    //�C���f�b�N�X�f�[�^��GPU�ɑ��M
    void* indexDataBegin;
    meshData.indexBuffer->Map(0, &readRange, &indexDataBegin);
    memcpy(indexDataBegin, indices.data(), indexBufferSize);
    meshData.indexBuffer->Unmap(0, nullptr);

    meshData.indexBufferView.BufferLocation = meshData.indexBuffer->GetGPUVirtualAddress();
    meshData.indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    meshData.indexBufferView.SizeInBytes = indexBufferSize;

    meshData.indexCount = static_cast<UINT>(indices.size());

    //�{�[�����̃��\�[�X���쐬
    CD3DX12_RESOURCE_DESC d = CD3DX12_RESOURCE_DESC::Buffer(sizeof(XMMATRIX) * boneInfos.size());

    HRESULT hr = m_pDevice->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &d,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_boneMatricesBuffer));
    if (FAILED(hr)) {
        printf("�{�[���o�b�t�@�̐����Ɏ��s���܂����B%1x\n", hr);
    }

    return meshData;
}

void Character::LoadBones(const aiScene* scene, Mesh& meshStruct, aiMesh* mesh, std::vector<Vertex>& vertices)
{
    for (UINT i = 0; i < mesh->mNumBones; i++) {
        aiBone* bone = mesh->mBones[i];
        UINT boneIndex = 0;

        // �{�[�����܂��o�^����Ă��Ȃ��ꍇ�A�}�b�s���O��ǉ�
        if (boneMapping.find(bone->mName.C_Str()) == boneMapping.end()) {
            //�ǉ�����{�[���̃C���f�b�N�X���擾
            boneIndex = static_cast<UINT>(boneInfos.size());

            //���_���猩�āA�{�[�������݂���ʒu(�I�t�Z�b�g)���擾
            XMMATRIX boneOffset = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<XMFLOAT4X4*>(&bone->mOffsetMatrix)));

            //�Ȃ����I�t�Z�b�g�̃v���X�}�C�i�X�����]���Ă��܂��{�[�������邽�߁A����(Z��)�̂ݏ�Ƀv���X�֏C��
            if (boneOffset.r[3].m128_f32[1] < 0.0f)
                boneOffset.r[3].m128_f32[1] = -boneOffset.r[3].m128_f32[1];

            //Y����Z��������  (�f�t�H���g��Z���������̂���)
            float boneOffsetY = boneOffset.r[3].m128_f32[2];
            boneOffset.r[3].m128_f32[2] = boneOffset.r[3].m128_f32[1];
            boneOffset.r[3].m128_f32[1] = boneOffsetY;

            //�{�[�����쐬
            Bone boneChild(bone->mName.C_Str(), boneOffset);

            XMMATRIX m = boneOffset;
            if (boneChild.GetBoneName() == "Chest") {
                printf("ChestMatrix = \n");
                for (int i = 0; i < 4; i++) {
                    printf("%f, %f, %f, %f\n", m.r[i].m128_f32[0], m.r[i].m128_f32[1], m.r[i].m128_f32[2], m.r[i].m128_f32[3]);
                }
            }
            if (boneChild.GetBoneName() == "Left arm") {
                printf("LeftArmMatrix = \n");
                for (int i = 0; i < 4; i++) {
                    printf("%f, %f, %f, %f\n", m.r[i].m128_f32[0], m.r[i].m128_f32[1], m.r[i].m128_f32[2], m.r[i].m128_f32[3]);
                }
            }
            if (boneChild.GetBoneName() == "Left leg") {
                printf("HeadMatrix = \n");
                for (int i = 0; i < 4; i++) {
                    printf("%f, %f, %f, %f\n", m.r[i].m128_f32[0], m.r[i].m128_f32[1], m.r[i].m128_f32[2], m.r[i].m128_f32[3]);
                }
            }

            //�z��ɒǉ�
            boneInfos.push_back(XMMatrixIdentity());
            bones.push_back(boneChild);

            //�{�[�����ƃC���f�b�N�X��R�Â�
            boneMapping[bone->mName.C_Str()] = boneIndex;
        }
        else {
            boneIndex = boneMapping[bone->mName.C_Str()];
        }

        //�{�[���̉e�����󂯂钸�_�̐��Ԃ�J��Ԃ�
        for (UINT j = 0; j < bone->mNumWeights; j++) {
            //���_�C���f�b�N�X
            UINT vertexID = bone->mWeights[j].mVertexId;
            //���̒��_���󂯂�E�F�C�g
            float weight = bone->mWeights[j].mWeight;

            //���_���擾
            Vertex& vertex = vertices[vertexID];

            if (weight >= 1.00f && !bones[boneIndex].bInited) {
                bones[boneIndex].m_boneOffset = XMMatrixTranslation(vertex.Position.x, vertex.Position.y, vertex.Position.z);
                bones[boneIndex].bInited = true;
            }

            //�󂢂Ă���{�[���E�F�C�g�X���b�g��T���B (�ő�4�̃{�[������e�����󂯂�)
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
                // 4�ȏ�̃{�[���E�F�C�g�̓T�|�[�g�O
                //printf("���_ %d �� 4 �ȏ�̃{�[���E�F�C�g�������Ă��܂��B\n", vertexID);
            }
        }
    }
}

//�{�[���̐e�q�֌W���擾
void Character::LoadBoneFamily(const aiNode* node)
{
    if (boneMapping.find(node->mName.C_Str()) != boneMapping.end()) {
        UINT boneIndex = boneMapping[node->mName.C_Str()];
        for (UINT i = 0; i < node->mNumChildren; i++) {
            if (boneMapping.find(node->mChildren[i]->mName.C_Str()) != boneMapping.end()) {
                UINT childIndex = boneMapping[node->mChildren[i]->mName.C_Str()];

                //�q�{�[���Ɛe�{�[����ݒ�
                bones[boneIndex].AddChildBone(&bones[childIndex], childIndex);
                bones[childIndex].SetParentBone(boneIndex);
            }
        }
    }

    //�q�{�[�������l�ɐݒ�
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
void Character::UpdateBoneRotation(std::string boneName, XMFLOAT3& rotation)
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

    //�e�{�[�������݂���ꍇ�A���̃��[���h���W��K�p
    if (bones[boneIndex].GetParentBoneIndex() != UINT32_MAX) {
        parentTransform = boneInfos[bones[boneIndex].GetParentBoneIndex()];
    }

    //�{�[���̍ŏI�I�Ȉʒu�A��]�A�X�P�[��������

    Bone& bone = bones[boneIndex];
    XMMATRIX scale = XMMatrixScaling(bone.m_scale.x, bone.m_scale.y, bone.m_scale.z);

    XMMATRIX rotX = XMMatrixRotationX(XMConvertToRadians(bone.m_rotation.x));  //Unity��ɂ��邽�߁A�l�𔽓]
    XMMATRIX rotY = XMMatrixRotationY(XMConvertToRadians(bone.m_rotation.z));  //Unity��ɂ��邽�߁AY����Z�������ւ��AZ���𔽓]
    XMMATRIX rotZ = XMMatrixRotationZ(XMConvertToRadians(bone.m_rotation.y));  //Y���͂��̂܂�
    XMMATRIX rot = rotX * rotY * rotZ;
    //XMVECTOR rotVec = XMQuaternionRotationRollPitchYaw(-bone.m_rotation.x, bone.m_rotation.z, -bone.m_rotation.y);
    //XMMATRIX rot = XMMatrixRotationQuaternion(rotVec);

    XMMATRIX pos = XMMatrixTranslation(bone.m_position.x, bone.m_position.y, bone.m_position.z);
    //XMMATRIX offsetBack = XMMatrixTranslation(bone.GetBoneOffset().r[3].m128_f32[0], bone.GetBoneOffset().r[3].m128_f32[1], bone.GetBoneOffset().r[3].m128_f32[2]);
    //XMMATRIX offsetOrigin = XMMatrixTranslation(-bone.GetBoneOffset().r[3].m128_f32[0], -bone.GetBoneOffset().r[3].m128_f32[1], -bone.GetBoneOffset().r[3].m128_f32[2]);
    XMMATRIX offsetBack = XMMatrixInverse(nullptr, bone.GetBoneOffset());

    XMMATRIX boneTransform = scale * offsetBack * rot * bone.GetBoneOffset() * pos;

    //�e�̃��[���h�ϊ��ƃ��[�J���ϊ�������
    XMMATRIX finalTransform = parentTransform * XMMatrixTranspose(boneTransform);

    //�V�F�[�_�[�ɑ��郏�[���h���W�ɑ��
    boneInfos[boneIndex] = finalTransform;

    // �q�{�[���ɂ��ϊ���`�d
    for (UINT i = 0; i < bone.GetChildBoneCount(); i++) {
        UpdateBoneTransform(bone.GetChildBoneIndex(i), boneInfos[boneIndex]);
    }
}

void Character::UpdateBoneTransform()
{
    //�{�[���̍ŏI�ʒu������
    XMMATRIX m = XMMatrixIdentity();
    UpdateBoneTransform(0, m);

    //�{�[���o�b�t�@�ɑ��M
    void* pData;
    HRESULT hr = m_boneMatricesBuffer->Map(0, nullptr, &pData);
    if (SUCCEEDED(hr))
    {
        memcpy(pData, boneInfos.data(), sizeof(XMMATRIX) * boneInfos.size()); // �{�[���}�g���b�N�X���R�s�[
        m_boneMatricesBuffer->Unmap(0, nullptr);
    }
}
