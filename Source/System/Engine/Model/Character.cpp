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
    printf("CreateConstantBuffer - %dms\n", tempTime);

    BinaryReader br(modelFile);
    char* headerBuf = br.ReadBytes(br.ReadByte());
    std::string header = headerBuf;
    delete[] headerBuf;
    br.Close();

    //�w�b�_�[��hcs�t�@�C����������
    if (header == std::string(MODEL_HEADER)) {
        LoadHCS(modelFile);
    }
    else {
        LoadFBX(modelFile);
    }

    tempTime = timeGetTime();

    m_boneManager.UpdateBoneMatrix();

    DWORD loadTime = timeGetTime() - startTime;
	printf("���f��:%s�����[�h���܂����B %dms\n", modelFile.c_str(), loadTime);

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
	//�A�j���[�V������1�����Ȃ��ꍇ�́A���݂̃A�j���[�V�����C���f�b�N�X��0�ɂ���
    if (m_characterAnimations.size() == 1) {
        m_nowAnimationIndex = 0;
    }
    return (unsigned int)(m_characterAnimations.size() - 1);
}

void Character::Update()
{
    //�A�j���[�V�����̍X�V
    UpdateAnimation();

    //�{�[���̃}�g���b�N�X���X�V
    m_boneManager.UpdateBoneMatrix();

    //�V�F�C�v�L�[�̃E�F�C�g���X�V
    UpdateShapeKeys();
}

void Character::LateUpdate(UINT backBufferIndex)
{
    //�{�[���o�b�t�@�ɑ��M
    memcpy(m_pBoneMatricesMap, m_boneManager.m_boneInfos.data(), sizeof(XMMATRIX) * m_boneManager.m_boneInfos.size()); //�{�[���}�g���b�N�X���R�s�[

    Model::LateUpdate(backBufferIndex);

    //�L�����N�^�[�̓��[�g���[�V�������l��
    XMFLOAT3 armaturePos = m_boneManager.m_armatureBone.m_position;
    armaturePos.x = armaturePos.x;
    armaturePos.z = -armaturePos.z;
    XMFLOAT3 hipPos = XMFLOAT3(0.0f, 0.0f, 0.0f);
    if (m_boneManager.m_bones.size() > 0) {
        //m_bones��0�Ԗڂ͊�{�I�Ƀ��[�g�{�[��
        hipPos = m_boneManager.m_bones[0].m_position;
        float temp = hipPos.x;
        hipPos.x = -hipPos.z;
        hipPos.z = -temp;
    }
    //�J�����ʒu����I�u�W�F�N�g�ʒu�܂ł̋�����m_depth�ɐݒ�
    XMFLOAT3 tempPos = armaturePos + hipPos + m_position;
    XMFLOAT3 camPos;
    XMStoreFloat3(&camPos, m_pCamera->m_eyePos);
    m_depth = DistanceSq(tempPos, camPos);
}

void Character::LoadFBX(const std::string& fbxFile)
{
    Assimp::Importer importer;

    //���f���ǂݍ��ݎ��̃t���O�B���b�V���̃|���S���͂��ׂĎO�p�`�ɂ��A�{�[�������݂���ꍇ�͉e�����󂯂�E�F�C�g��4�܂łɂ���
    UINT flag = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
        aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights | aiProcess_OptimizeMeshes;
    const aiScene* scene = importer.ReadFile(fbxFile, flag);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        printf("FBX�t�@�C�������[�h�ł��܂���ł����B\n");
        return;
    }

    ProcessNode(scene, scene->mRootNode);

    //�{�[���ɐe�q�֌W��t����
    LoadBoneFamily(scene->mRootNode);

    XMMATRIX mat = XMMatrixIdentity();
    CalculateBoneTransforms(scene->mRootNode, mat);

    for (HumanoidMesh& humanoidMesh : m_humanoidMeshes) {
        CreateHumanoidMeshBuffer(humanoidMesh);
    }

    //�V�F�C�v�L�[�̈ʒu����ێ����郊�\�[�X���쐬
    CreateShapeDeltasTexture(false);

    //�}�e���A���̓ǂݍ���
    for (size_t i = 0; i < scene->mNumMeshes; i++)
    {
        m_meshes[i]->pModel = this;

        aiMesh* mesh = scene->mMeshes[i];
        //���b�V���̃}�e���A�����擾����
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            //�t�@�C���� = �}�e���A���� + .png
            std::string nameOnly = material->GetName().C_Str() + std::string(".png");

            SetTexture(m_meshes[i], nameOnly);
        }
        else {
            //�e�N�X�`�������݂��Ȃ��ꍇ�͔��P�F�e�N�X�`�����g�p
            m_meshes[i]->pMaterial = m_pMaterialManager->AddMaterialWithShapeData("BoneWhite", m_meshes[i]->meshData->shapeDataIndex);
        }
    }
}

void Character::LoadHCS(const std::string& hcsFile)
{
    DWORD startTime = timeGetTime();
    //�t�@�C�������݂��Ȃ�
    if (!std::filesystem::exists(hcsFile)) {
        return;
    }

    //�o�C�i���t�@�C���Ƃ��ĊJ��
    BinaryReader br(hcsFile);

    //�w�b�_�[
    char* headerBuf = br.ReadBytes(br.ReadByte());
	std::string header = headerBuf;
    delete[] headerBuf;

    //�w�b�_�[�����f���`���łȂ��ꍇ
    if (header != std::string(MODEL_HEADER)) {
        br.Close();
        return;
    }

    //���f���̃t�H�[�}�b�g
    BYTE format = br.ReadByte();
    //�L�����N�^�[�ȊO�͏������Ȃ�
    if (format != MODEL_CHARACTER) {
        br.Close();
        return;
    }

	//�{�[���A�V�F�C�v�L�[�A���b�V���̓ǂݍ���
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
        ProcessMesh(br, m_humanoidMeshes[i], i);
        m_meshes.push_back(m_humanoidMeshes[i].pMesh);
    }

    endTime = timeGetTime();
    printf("ProcessMesh - %dms\n", endTime - startTime);
    startTime = endTime;

    for (HumanoidMesh& humanoidMesh : m_humanoidMeshes) {
        CreateHumanoidMeshBuffer(humanoidMesh);
    }

    //�V�F�C�v�L�[�̈ʒu����ێ����郊�\�[�X���쐬
    CreateShapeDeltasTexture(true);

    endTime = timeGetTime();
    printf("CreateHumanoidMeshBuffer - %dms\n", endTime - startTime);
    startTime = endTime;

    //�}�e���A���̓ǂݍ���
    for (UINT i = 0; i < meshCount; i++) {
        bool bExistTexture = br.ReadBoolean();

        m_meshes[i]->pModel = this;

        //���b�V���̃}�e���A�����擾����
        if (bExistTexture) {
            char* nameBuf = br.ReadBytes(br.ReadByte());
            std::string nameOnly = nameBuf;
            delete[] nameBuf;

            SetTexture(m_meshes[i], nameOnly);
        }
        else {
            //�e�N�X�`�������݂��Ȃ��ꍇ�͔��P�F�e�N�X�`�����g�p
            m_meshes[i]->pMaterial = m_pMaterialManager->AddMaterialWithShapeData("BoneWhite", m_meshes[i]->meshData->shapeDataIndex);
        }
    }

    m_bHCSFile = true;

    endTime = timeGetTime();
    printf("SetTexture - %dms\n", endTime - startTime);
}

void Character::ProcessNode(const aiScene* pScene, const aiNode* pNode)
{
    //�m�[�h�Ƀ��b�V�����܂܂�Ă�����ǂݍ���
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

    //���_�̏���
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

    //�C���f�b�N�X�̏���
    for (UINT i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (UINT j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    //���_����ۑ�
    humanoidMesh.vertexCount = static_cast<UINT>(vertices.size());

    Mesh* meshData = new Mesh();
    humanoidMesh.pMesh = meshData;

    //�{�[���̏���
    LoadBones(mesh, vertices);

    //�V�F�C�v�L�[�̏���
    LoadShapeKey(mesh, vertices, humanoidMesh);

    //�V�F�[�_�[�ɕK�v�ȃo�b�t�@�𐶐�
    Model::CreateBuffer(meshData, vertices, indices, sizeof(Vertex), meshIndex);
    CreateBuffer(humanoidMesh);

    return meshData;
}

void Character::ProcessMesh(BinaryReader& br, HumanoidMesh& humanoidMesh, UINT meshIndex)
{
    UINT meshBufferOriginalSize = br.ReadUInt32();
    UINT meshBufferCompressedSize = br.ReadUInt32();

    char* compressedBuffer = br.ReadBytes(meshBufferCompressedSize);

    //���ɓ������f�������[�h����Ă���΁A������Q��
    if (s_sharedMeshes.find(m_modelFile) != s_sharedMeshes.end() && s_sharedMeshes[m_modelFile].meshDataList.size() > meshIndex) {
        std::vector<Vertex> vertex;
        std::vector<UINT> index;
        std::shared_ptr<MeshData> meshData = s_sharedMeshes[m_modelFile].meshDataList[meshIndex];
        Model::CreateBuffer(humanoidMesh.pMesh, vertex, index, sizeof(Vertex), meshIndex);

        //���_����ۑ�
        humanoidMesh.vertexCount = meshData->vertexCount;
        CreateBuffer(humanoidMesh);

        delete[] compressedBuffer;

        return;
    }

    //���k����Ă���{�[��������
    std::vector<char> meshBuffer;
    BinaryDecompress(meshBuffer, meshBufferOriginalSize, compressedBuffer, meshBufferCompressedSize);

    delete[] compressedBuffer;

    BinaryReader meshReader(meshBuffer);

    //�𓀂����o�b�t�@��ǂݍ���
    std::vector<Vertex> vertices;
    std::vector<UINT> indices;

    //���_�̏���
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

    //�C���f�b�N�X�̏���
    UINT faceCount = meshReader.ReadUInt32();
    for (UINT i = 0; i < faceCount; i++) {
        UINT indexCount = meshReader.ReadUInt32();
        for (UINT j = 0; j < indexCount; j++) {
            indices.push_back(meshReader.ReadUInt32());
        }
    }

    //�e���_�̃{�[���̉e���x������
    UINT boneCount = meshReader.ReadUInt32();
    for (UINT i = 0; i < boneCount; i++) {
        //�{�[��������index���擾
        char* boneNameBuf = meshReader.ReadBytes(meshReader.ReadByte());
        std::string boneName = boneNameBuf;
        delete[] boneNameBuf;

        UINT boneIndex = m_boneManager.m_boneMapping[boneName];

        //�e�����󂯂钸�_�̐�
        UINT weightCount = meshReader.ReadUInt32();
        for (UINT j = 0; j < weightCount; j++) {
            UINT vertexID = meshReader.ReadUInt32();
            float weight = meshReader.ReadFloat();

            //���_���擾
            Vertex& vertex = vertices[vertexID];

            //�󂢂Ă���{�[���E�F�C�g�X���b�g��T���B (�ő�4�̃{�[������e�����󂯂�)
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
                //4�ȏ�̃{�[���E�F�C�g�̓T�|�[�g�O
                //printf("���_ %d �� 4 �ȏ�̃{�[���E�F�C�g�������Ă��܂��B\n", vertexID);
            }
        }
    }

    //���_����ۑ�
    humanoidMesh.vertexCount = static_cast<UINT>(vertices.size());

    //�V�F�[�_�[�ɕK�v�ȃo�b�t�@�𐶐�
    Model::CreateBuffer(humanoidMesh.pMesh, vertices, indices, sizeof(Vertex), meshIndex);
    CreateBuffer(humanoidMesh);
}

void Character::CreateBuffer(HumanoidMesh& humanoidMesh)
{
    //�q�[�v�ݒ�
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    //�{�[�����̃��\�[�X���쐬
    CD3DX12_RESOURCE_DESC boneBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(XMMATRIX) * m_boneManager.m_boneInfos.size());
    HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &boneBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_boneMatricesBuffer));
    if (FAILED(hr)) {
        printf("�{�[���o�b�t�@�̐����Ɏ��s���܂����B\n");
    }

    m_boneMatricesBuffer->Map(0, nullptr, &m_pBoneMatricesMap);

    //���_���ƃV�F�C�v�L�[����ێ�
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
    //�q�[�v�ݒ�
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    //�V�F�C�v�L�[�̏���ێ�����p�̃��\�[�X���쐬
    if (humanoidMesh.shapeWeights.size() > 0) {
        //�V�F�C�v�L�[�̃E�F�C�g����ێ����郊�\�[�X���쐬
        UINT64 shapeWeightsSize = sizeof(float) * humanoidMesh.shapeWeights.size();
        D3D12_RESOURCE_DESC shapeWeightsBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(shapeWeightsSize);
        HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &shapeWeightsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&humanoidMesh.pMesh->shapeWeightsBuffer));
        if (FAILED(hr)) {
            printf("�V�F�C�v�L�[�E�F�C�g�o�b�t�@�̐����Ɏ��s���܂����B%ld\n", hr);
            HRESULT removedReason = m_pDevice->GetDeviceRemovedReason();
            printf("Device removed reason:%ld\n", removedReason);
        }

        //�E�F�C�g���̏����l������ (Update�֐��ŏ�ɍX�V�����)
        humanoidMesh.pMesh->shapeWeightsBuffer->Map(0, nullptr, &humanoidMesh.pMesh->pShapeWeightsMapped);
        memcpy(humanoidMesh.pMesh->pShapeWeightsMapped, humanoidMesh.shapeWeights.data(), shapeWeightsSize);
    }
}

void Character::CreateShapeDeltasTexture(bool bClearDeltas)
{
    //���C���X���b�h�ŕK�v�ȃA�b�v���[�h�o�b�t�@���쐬
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

        UINT maxWidth = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION; //�����̍ő� = 16384
        UINT width = (vertexCount > maxWidth) ? maxWidth : static_cast<UINT>(vertexCount);
        UINT height = static_cast<UINT>(((vertexCount + width - 1) / width) * shapeCount);

        size_t rowPitch = (width * sizeof(XMFLOAT4) + 255) & ~255; //256�̔{���ɂȂ�悤��

        shapeBuffers[i] = g_resourceCopy->CreateUploadBuffer(rowPitch * height);

        if (humanoidMesh.pMesh->meshData->shapeDeltasBuffer) {
            if (shapeBuffers[i]) {
                shapeBuffers[i]->Release();
                shapeBuffers[i] = nullptr;
            }
            continue;
        }

        //�V�F�C�v�L�[�p�̃��\�[�X���쐬
        CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT, static_cast<UINT>(width), static_cast<UINT>(height), 1, 1);
        CD3DX12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COPY_DEST;
        HRESULT result = m_pDevice->CreateCommittedResource(&texHeapProp, D3D12_HEAP_FLAG_NONE, &resDesc, state, nullptr, IID_PPV_ARGS(&humanoidMesh.pMesh->meshData->shapeDeltasBuffer));
        if (FAILED(result)) {
            printf("�V�F�C�v�L�[�p�̃��\�|�X�̍쐬�Ɏ��s���܂����B: �G���[�R�[�h = %1x\n", result);
        }
    }

    //�ʃX���b�h�ŃA�b�v���[�h�o�b�t�@�Ƀf�[�^���R�s�[���AGPU�ɑ��M
    std::thread([=] {
        for (UINT i = 0; i < static_cast<UINT>(m_humanoidMeshes.size()); i++) {
            HumanoidMesh& humanoidMesh = m_humanoidMeshes[i];

            if (!shapeBuffers[i]) {
                if (bClearDeltas) {
                    humanoidMesh.shapeDeltas.clear();
                }
                continue;
            }


            //�K�v�ȃT�C�Y���v�Z
            size_t vertexCount = humanoidMesh.vertexCount;
            size_t shapeCount = humanoidMesh.shapeWeights.size();

            if (shapeCount <= 0) {
                if (bClearDeltas) {
                    humanoidMesh.shapeDeltas.clear();
                }
                continue;
            }

            g_resourceCopy->BeginCopyResource();

            UINT maxWidth = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION; //�����̍ő� = 16384
            UINT width = (vertexCount > maxWidth) ? maxWidth : static_cast<UINT>(vertexCount);
            UINT height = static_cast<UINT>(((vertexCount + width - 1) / width) * shapeCount);

            size_t rowPitch = (width * sizeof(XMFLOAT4) + 255) & ~255; //256�̔{���ɂȂ�悤��
            size_t tempWidth = rowPitch / sizeof(XMFLOAT4);
            std::vector<XMFLOAT4> textureData(tempWidth * height, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f)); //������

            //�V�F�C�v�L�[�����e�N�X�`���ɕϊ�
            for (size_t shapeID = 0; shapeID < shapeCount; shapeID++) {
                for (size_t vertexID = 0; vertexID < vertexCount; vertexID++) {
                    size_t x = vertexID % tempWidth;
                    size_t y = (vertexID / tempWidth) * shapeCount + shapeID; //�c�ʒu

                    //�͈̓`�F�b�N
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
                printf("UploadBuffer�̃}�b�v�Ɏ��s���܂����B\n");
                return;
            }
            memcpy(mappedData, textureData.data(), rowPitch * height);
            shapeBuffers[i]->Unmap(0, nullptr);

            //�R�s�[���ݒ�
            D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
            dstLocation.pResource = humanoidMesh.pMesh->meshData->shapeDeltasBuffer.Get();
            dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dstLocation.SubresourceIndex = 0;

            //�R�s�[����ݒ�
            D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
            srcLocation.pResource = shapeBuffers[i];
            srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            srcLocation.PlacedFootprint.Footprint.Width = width;
            srcLocation.PlacedFootprint.Footprint.Height = height;
            srcLocation.PlacedFootprint.Footprint.Depth = 1;
            srcLocation.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(rowPitch);
            srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            g_resourceCopy->GetCommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

            //���\�[�X�o���A��STATE��ύX
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
    //�{�[����boneMap�ɑ��݂��邩�`�F�b�N
    Bone* pBone = m_boneManager.GetBone(node->mName.C_Str());
    XMMATRIX nodeTransform = XMMatrixIdentity();

    if (pBone) {
        //�{�[���̃��[�J���ϊ��s����擾
        nodeTransform = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&node->mTransformation)));
        //printf("NodeName = %s, x=%f, y=%f, z=%f\n", node->mName.C_Str(), nodeTransform.r[3].m128_f32[0], nodeTransform.r[3].m128_f32[1], nodeTransform.r[3].m128_f32[2]);
    }

    //�e�{�[���̕ϊ��s��Ƃ̍����i�e����q�ւ̕ϊ��j
    XMMATRIX globalTransform = nodeTransform * parentTransform;

    //�{�[���̃��[���h��Ԃł̕ϊ��s���ۑ��iSphere��u�����߂̈ʒu�Ƃ��Ďg�p�j
    if (pBone) {
        pBone->SetBoneOffset(globalTransform);
    }

    //�q�m�[�h�ɑ΂��čċA�I�ɏ��������s
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

        //�{�[�����܂��o�^����Ă��Ȃ��ꍇ�A�}�b�s���O��ǉ�
        if (!m_boneManager.Exist(boneName)) {
            //�ǉ�����{�[���̃C���f�b�N�X���擾
            boneIndex = static_cast<UINT>(m_boneManager.m_boneInfos.size());

            //�{�[�����쐬
            Bone* pBoneChild = m_boneManager.AddBone(boneName, boneIndex);

            //��⑫�͍��E�𔻒� (��U��̔�������A���̏ꍇ��LoadBoneFamily���ŕύX)
            if (boneName[0] == 'L' || boneName.at(boneName.size() - 1) == 'L') {
                pBoneChild->m_boneType = BONETYPE_LEFT_ARM;
            }
            if (boneName[0] == 'R' || boneName.at(boneName.size() - 1) == 'R') {
                pBoneChild->m_boneType = BONETYPE_RIGHT_ARM;
            }

            //�ڂ̃{�[���͍��E�̋�ʂ����Ȃ�
            if (boneName.find("Eye") != std::string::npos || boneName.find("eye") != std::string::npos) {
                pBoneChild->m_boneType = BONETYPE_DEFAULT;
            }
        }
        else {
            //�����̃{�[�������݂���ꍇ������g�p (�قȂ郁�b�V���Ԃœ����̃{�[���̏ꍇ�A�قƂ�ǂ����L����Ă��邽��)
            boneIndex = m_boneManager.m_boneMapping[bone->mName.C_Str()];
        }

        //�{�[���̉e�����󂯂钸�_�̐��Ԃ�J��Ԃ�
        for (UINT j = 0; j < bone->mNumWeights; j++) {
            //���_�C���f�b�N�X
            UINT vertexID = bone->mWeights[j].mVertexId;
            //���̒��_���ǂ̂��炢�e�����󂯂邩
            float weight = bone->mWeights[j].mWeight;

            //���_���擾
            Vertex& vertex = vertices[vertexID];

            //�󂢂Ă���{�[���E�F�C�g�X���b�g��T���B (�ő�4�̃{�[������e�����󂯂�)
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
                //4�ȏ�̃{�[���E�F�C�g�̓T�|�[�g�O
                //printf("���_ %d �� 4 �ȏ�̃{�[���E�F�C�g�������Ă��܂��B\n", vertexID);
            }
        }
    }
}

void Character::LoadBones(BinaryReader& br)
{
    UINT boneBufferOriginalSize = br.ReadUInt32();
	UINT boneBufferCompressedSize = br.ReadUInt32();
	char* compressedBuffer = br.ReadBytes(boneBufferCompressedSize);

	//���k����Ă���{�[��������
    std::vector<char> boneBuffer;
    BinaryDecompress(boneBuffer, boneBufferOriginalSize, compressedBuffer, boneBufferCompressedSize);

    delete[] compressedBuffer;

	//�𓀂����o�b�t�@��ǂݍ���
	BinaryReader boneReader(boneBuffer);

    UINT boneCount = boneReader.ReadUInt32();
    for (UINT i = 0; i < boneCount; i++) {
        UINT boneIndex = static_cast<UINT>(m_boneManager.m_bones.size());

        //�{�[����
        char* boneNameBuf = boneReader.ReadBytes(boneReader.ReadByte());
        std::string boneName = boneNameBuf;
        delete[] boneNameBuf;

        //�{�[���̏����ʒu
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

//�{�[���̐e�q�֌W���擾
void Character::LoadBoneFamily(const aiNode* node)
{
    //�m�[�h(Armature�{�[��)�̏ォ�牺�֎��s�����

    //�m�[�h���̃{�[��������
    if (m_boneManager.Exist(node->mName.C_Str())) {
        UINT boneIndex = m_boneManager.m_boneMapping[node->mName.C_Str()];
        //�m�[�h�̎q���
        for (UINT i = 0; i < node->mNumChildren; i++) {
            //�q�m�[�h�̖��O�̃{�[��������
            if (m_boneManager.m_boneMapping.find(node->mChildren[i]->mName.C_Str()) != m_boneManager.m_boneMapping.end()) {
                UINT childIndex = m_boneManager.m_boneMapping[node->mChildren[i]->mName.C_Str()];

                //�q�{�[���Ɛe�{�[����ݒ�
                m_boneManager.m_bones[boneIndex].AddChildBone(&m_boneManager.m_bones[childIndex], childIndex);
                m_boneManager.m_bones[childIndex].SetParentBone(&m_boneManager.m_bones[boneIndex], boneIndex);

                std::string boneName = m_boneManager.m_bones[childIndex].GetBoneName();
                //���̏ꍇBONETYPE_LEFT(RIGHT)_LEG�ɕύX (LoadBones()�ő���BONETYPE_LEFT_ARM�Ǝw�肳��邽��)
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

                //�q�{�[���͐e�{�[���̎�ނ��p��
                if (m_boneManager.m_bones[boneIndex].m_boneType != BONETYPE_DEFAULT) {
                    m_boneManager.m_bones[childIndex].m_boneType = m_boneManager.m_bones[boneIndex].m_boneType;
                }
            }
        }
    }

    //�q�{�[�������l�ɐݒ�
    for (UINT i = 0; i < node->mNumChildren; i++) {
        LoadBoneFamily(node->mChildren[i]);
    }
}

void Character::LoadShapeKey(const aiMesh* mesh, std::vector<Vertex>& vertices, HumanoidMesh& humanoidMesh)
{
    //���b�V���Ɋ܂܂��V�F�C�v�L�[�ꗗ
    for (UINT i = 0; i < mesh->mNumAnimMeshes; i++) {
        aiAnimMesh* animMesh = mesh->mAnimMeshes[i];

        std::string shapeName = UTF8ToShiftJIS(animMesh->mName.C_Str());

        //�����̃V�F�C�v�L�[�����݂���Ζ��� (�قڂȂ�����)
        if (humanoidMesh.shapeMapping.find(shapeName) == humanoidMesh.shapeMapping.end()) {
            UINT shapeIndex = static_cast<UINT>(humanoidMesh.shapeWeights.size());

            humanoidMesh.shapeWeights.push_back(0.0f);
            humanoidMesh.shapeMapping[shapeName] = shapeIndex;

            for (UINT j = 0; j < animMesh->mNumVertices; j++) {
                aiVector3D& vec = animMesh->mVertices[j];
                //�V�F�C�v�L�[�́A100%�̂Ƃ��̂��̒��_�̈ʒu
                XMFLOAT3 shapePos = XMFLOAT3(vec.x, vec.y, vec.z);
                //���̈ʒu������
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

    //���k����Ă���V�F�C�v������
    std::vector<char> humanoidBuffer;
    BinaryDecompress(humanoidBuffer, humanoidBufferOriginalSize, compressedBuffer, humanoidBufferCompressedSize);

    delete[] compressedBuffer;

    //�𓀂����o�b�t�@��ǂݍ���
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

        //���b�V����
        char* meshNameBuf = humanoidReader.ReadBytes(humanoidReader.ReadByte());
        humanoidMesh.pMesh->meshName = meshNameBuf;
        humanoidList.meshNames[i] = meshNameBuf;
        delete[] meshNameBuf;

        UINT shapeMappingCount = humanoidReader.ReadUInt32();
        humanoidList.shapeCounts[i] = shapeMappingCount;
        for (UINT j = 0; j < shapeMappingCount; j++) {
            //�V�F�C�v�L�[��
            char* shapeNameBuf = humanoidReader.ReadBytes(humanoidReader.ReadByte());
            std::string shapeName = shapeNameBuf;

            //.��؂�œ��������������Ă���ꍇ�폜
            std::vector<std::string> sprits = GetSprits(shapeName, '.');
            if (sprits.size() == 2) {
                shapeName = sprits[0];
            }

            delete[] shapeNameBuf;

            //�V�F�C�v�L�[�̃C���f�b�N�X
            UINT shapeIndex = humanoidReader.ReadUInt32();

            humanoidMesh.shapeWeights.push_back(0.0f);
            humanoidMesh.shapeMapping[shapeName] = shapeIndex;
            humanoidList.shapeMappings[i].emplace(shapeName, shapeIndex);
        }
        //�V�F�C�v�L�[�́A100%�̂Ƃ��̂��̒��_�̈ʒu
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

    //�}�e���A�����쐬
    bool bExist;
    pMesh->pMaterial = m_pMaterialManager->AddMaterialWithShapeData(texPath, bExist, pMesh->meshData->shapeDataIndex);
    if (!bExist) {
        pMesh->pMaterial->SetMainTexture(texPath);
    }
}

//�{�[���̈ʒu���X�V
void Character::UpdateBonePosition(std::string boneName, XMFLOAT3& position)
{
    if (!m_boneManager.Exist(boneName)) {
        return;
    }

    UINT boneIndex = m_boneManager.m_boneMapping[boneName];

    m_boneManager.m_bones[boneIndex].m_position = position;
}

//�{�[���̉�]��ύX
void Character::UpdateBoneRotation(std::string boneName, XMFLOAT4& rotation)
{
    if (!m_boneManager.Exist(boneName)) {
        return;
    }

    UINT boneIndex = m_boneManager.m_boneMapping[boneName];

    m_boneManager.m_bones[boneIndex].m_rotation = rotation;
}

//�{�[���̃X�P�[����ύX
void Character::UpdateBoneScale(std::string boneName, XMFLOAT3& scale)
{
    if (!m_boneManager.Exist(boneName)) {
        return;
    }

    UINT boneIndex = m_boneManager.m_boneMapping[boneName];

    m_boneManager.m_bones[boneIndex].m_scale = scale;
}

//�V�F�C�v�L�[�̃E�F�C�g���X�V
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
        //�V�F�C�v�o�b�t�@�ɑ��M
        if (mesh.pMesh->shapeWeightsBuffer && mesh.bChangeShapeValue) {
            memcpy(mesh.pMesh->pShapeWeightsMapped, mesh.shapeWeights.data(), sizeof(float) * mesh.shapeWeights.size()); //�e�V�F�C�v�L�[�̃E�F�C�g���R�s�[
            mesh.bChangeShapeValue = false;
        }
    }
}

void Character::UpdateAnimation()
{
    //�A�j���[�V���������݂��Ȃ�
    if (m_nowAnimationIndex < 0 || m_characterAnimations.size() <= m_nowAnimationIndex) {
        return;
    }

    //�L�[�t���[�������݂��Ȃ�
    if (m_characterAnimations[m_nowAnimationIndex].pAnimation->m_frames.size() <= 0) {
        return;
    }

    //�A�j���[�V�������Ԃ��X�V
    m_nowAnimationTime += *m_pFrameTime * m_animationSpeed;

    //���݂̃A�j���[�V�������Ԃ̃t���[�����擾
    CharacterAnimationFrame* pFrame = m_characterAnimations[m_nowAnimationIndex].pAnimation->GetCharacterFrame(m_nowAnimationTime, &m_characterAnimations[m_nowAnimationIndex].beforeFrameIndex);

    //�t���[�������݂��Ȃ���Ώ������I�� (��ɃA�j���[�V�������ǂݍ��܂�Ă��Ȃ��ꍇ)
    if (!pFrame) {
        return;
    }

    m_boneManager.m_armatureBone.m_position = pFrame->armatureAnimation.position;
    m_boneManager.m_armatureBone.m_rotation = pFrame->armatureAnimation.rotation;

    //�{�[���A�j���[�V����
    for (UINT i = 0; i < pFrame->boneAnimations.size(); i++) {
        std::string boneName = m_characterAnimations[m_nowAnimationIndex].pAnimation->m_boneMapping[i];
        UpdateBonePosition(boneName, pFrame->boneAnimations[i].position);
        UpdateBoneRotation(boneName, pFrame->boneAnimations[i].rotation);
    }

    //�V�F�C�v�L�[�̃A�j���[�V����
    for (UINT i = 0; i < m_characterAnimations[m_nowAnimationIndex].pAnimation->m_shapeNames.size(); i++) {
        std::string& shapeName = m_characterAnimations[m_nowAnimationIndex].pAnimation->m_shapeNames[i];
        if (pFrame->shapeAnimations.size() > i) {
            SetShapeWeight(shapeName, pFrame->shapeAnimations[i]);
        }
    }

    //�Đ����̃t���[�����Ō�̃t���[���������ꍇ�ŏ��ɖ߂�
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
