#include "Character.h"

static XMVECTOR ExtractEulerAngles(const XMMATRIX& matrix) {
    float pitch, yaw, roll;

    //Y�������̉�]�iyaw�j
    yaw = asinf(-matrix.r[2].m128_f32[0]);

    if (cosf(yaw) > 0.0001f) {
        //X����Z�������̉�]�ipitch �� roll�j
        pitch = atan2f(matrix.r[2].m128_f32[1], matrix.r[2].m128_f32[2]);
        roll = atan2f(matrix.r[1].m128_f32[0], matrix.r[0].m128_f32[0]);
    }
    else {
        //Y���� 90�x�܂��� -90�x�̎��́AGimbal Lock�ɑΉ�
        pitch = atan2f(-matrix.r[0].m128_f32[2], matrix.r[1].m128_f32[1]);
        roll = 0.0f;
    }

    return XMVectorSet(pitch, yaw, roll, 0.0f); //���W�A���p�ŏo��
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

    CreateConstantBuffer();

    DWORD tempTime = timeGetTime() - startTime;
    printf("CreateConstantBuffer - %dms\n", tempTime);

    BinaryReader br(modelFile);
    char* headerBuf = br.ReadBytes(br.ReadByte());
    std::string header = headerBuf;
    delete[] headerBuf;
    br.Close();

    if (header == std::string(MODEL_HEADER)) {
        LoadHCS(modelFile);
    }
    else {
        LoadFBX(modelFile);
    }

    tempTime = timeGetTime();

    for (HumanoidMesh& humanoidMesh : m_humanoidMeshes) {
        CreateHumanoidMeshBuffer(humanoidMesh);

        //HCS�t�@�C���̏ꍇ�̓V�F�C�v�L�[���N���A
        if (bHCSFile) {
            humanoidMesh.shapeDeltas.clear();
        }
    }
    printf("CreateHumanoidMeshBuffer - %dms\n", timeGetTime() - tempTime);

    tempTime = timeGetTime();
    //���[�g�V�O�l�`���ƃp�C�v���C���X�e�[�g��������
	m_pRootSignature = new RootSignature(m_pDevice, ShaderKinds::BoneShader);
	m_pPipelineState = new PipelineState(m_pDevice, m_pRootSignature);
    printf("PipelineState - %dms\n", timeGetTime() - tempTime);

    OnLoaded();

    DWORD loadTime = timeGetTime() - startTime;
	printf("���f��:%s�����[�h���܂����B %dms\n", modelFile.c_str(), loadTime);
}

Character::~Character()
{
}

UINT Character::AddAnimation(Animation animation)
{
    m_animations.push_back(animation);
	//�A�j���[�V������1�����Ȃ��ꍇ�́A���݂̃A�j���[�V�����C���f�b�N�X��0�ɂ���
    if (m_animations.size() == 1) {
        m_nowAnimationIndex = 0;
    }
    return (unsigned int)m_animations.size() - 1;
}

void Character::Update(UINT backBufferIndex)
{
    //�{�[���̃}�g���b�N�X���X�V
    UpdateBoneTransform();

    //�V�F�C�v�L�[�̃E�F�C�g���X�V
    UpdateShapeKeys();

    //�A�j���[�V�����̍X�V
    UpdateAnimation();

    //�e�N���X�̍X�V
    Model::Update(backBufferIndex);
}

void Character::LoadFBX(const std::string& fbxFile)
{
    Assimp::Importer importer;

    //���f���ǂݍ��ݎ��̃t���O�B���b�V���̃|���S���͂��ׂĎO�p�`�ɂ��A�{�[�������݂���ꍇ�͉e�����󂯂�E�F�C�g��4�܂łɂ���
    UINT flag = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
        aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights;
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

    //�}�e���A���̓ǂݍ���
    m_pDescriptorHeap = new DescriptorHeap(m_pDevice, scene->mNumMeshes, CHARACTER_DISCRIPTOR_HEAP_SIZE, ShadowSizeHigh);

    std::string dir = "Resource\\Model\\";
    for (size_t i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[i];
        //���b�V���̃}�e���A�����擾����
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            //�t�@�C���� = �}�e���A���� + .png
            std::string nameOnly = material->GetName().C_Str() + std::string(".png");

            SetTexture(nameOnly);
        }
        else {
            //�e�N�X�`�������݂��Ȃ��ꍇ�͔��P�F�e�N�X�`�����g�p
            Texture2D* pTexture = Texture2D::GetColor(1.0f, 1.0f, 1.0f);
            m_pDescriptorHeap->SetMainTexture(pTexture->Resource(), pTexture->Resource(), m_pShadowMapBuffer);
            m_textures.push_back(pTexture);
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
        Mesh* pMesh = ProcessMesh(br, m_humanoidMeshes[i]);
        m_meshes.push_back(pMesh);
    }

    endTime = timeGetTime();
    printf("ProcessMesh - %dms\n", endTime - startTime);
    startTime = endTime;

    //�}�e���A���̓ǂݍ���
    m_pDescriptorHeap = new DescriptorHeap(m_pDevice, meshCount, CHARACTER_DISCRIPTOR_HEAP_SIZE, ShadowSizeHigh);

    for (UINT i = 0; i < meshCount; i++) {
        bool bExistTexture = br.ReadBoolean();

        //���b�V���̃}�e���A�����擾����
        if (bExistTexture) {
            char* nameBuf = br.ReadBytes(br.ReadByte());
            std::string nameOnly = nameBuf;
            delete[] nameBuf;

            SetTexture(nameOnly);
        }
        else {
			//�e�N�X�`�������݂��Ȃ��ꍇ�͔��P�F�e�N�X�`�����g�p
            Texture2D* pTexture = Texture2D::GetColor(1.0f, 1.0f, 1.0f);
            m_pDescriptorHeap->SetMainTexture(pTexture->Resource(), pTexture->Resource(), m_pShadowMapBuffer);
			m_textures.push_back(pTexture);
        }
    }

    bHCSFile = true;

    endTime = timeGetTime();
    printf("SetTexture - %dms\n", endTime - startTime);
}

void Character::ProcessNode(const aiScene* pScene, const aiNode* pNode)
{
    //�m�[�h�Ƀ��b�V�����܂܂�Ă�����ǂݍ���
    for (UINT j = 0; j < pNode->mNumMeshes; j++) {
        aiMesh* mesh = pScene->mMeshes[pNode->mMeshes[j]];
        HumanoidMesh humanoidMesh{};
        humanoidMesh.meshName = UTF8ToShiftJIS(pNode->mName.C_Str());
        Mesh* pMesh = ProcessMesh(mesh, humanoidMesh);
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
    CreateBuffer(meshData, vertices, indices, humanoidMesh);

    return meshData;
}

Mesh* Character::ProcessMesh(BinaryReader& br, HumanoidMesh& humanoidMesh)
{
    UINT meshBufferOriginalSize = br.ReadUInt32();
    UINT meshBufferCompressedSize = br.ReadUInt32();
    char* compressedBuffer = br.ReadBytes(meshBufferCompressedSize);

    //���k����Ă���{�[��������
    std::vector<char> meshBuffer;
    BinaryDecompress(meshBuffer, meshBufferOriginalSize, compressedBuffer, meshBufferCompressedSize);

    delete[] compressedBuffer;

    //�𓀂����o�b�t�@��ǂݍ���
    BinaryReader meshReader(meshBuffer);

    std::vector<Vertex> vertices;
    std::vector<UINT> indices;

    //���_�̏���
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

    Mesh* meshData = new Mesh();
    humanoidMesh.pMesh = meshData;

    //�V�F�[�_�[�ɕK�v�ȃo�b�t�@�𐶐�
    CreateBuffer(meshData, vertices, indices, humanoidMesh);

    return meshData;
}

void Character::CreateBuffer(Mesh* pMesh, std::vector<Vertex>& vertices, std::vector<UINT>& indices, HumanoidMesh& humanoidMesh)
{
    Model::CreateBuffer(pMesh, vertices, indices, sizeof(Vertex));

    //�q�[�v�ݒ�
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    //�{�[�����̃��\�[�X���쐬
    CD3DX12_RESOURCE_DESC boneBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(XMMATRIX) * m_boneManager.m_boneInfos.size());
    HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &boneBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_boneMatricesBuffer));
    if (FAILED(hr)) {
        printf("�{�[���o�b�t�@�̐����Ɏ��s���܂����B\n");
    }

    //���_����ێ�����p�̃��\�[�X���쐬
    CD3DX12_RESOURCE_DESC contentsBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Contents));
    hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &contentsBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pMesh->contentsBuffer));
    if (FAILED(hr)) {
        printf("�R���e���c�o�b�t�@�̐����Ɏ��s���܂����B\n");
    }

    //���_���ƃV�F�C�v�L�[����ێ�
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
        void* pShapeWeightsBuffer;
        humanoidMesh.pMesh->shapeWeightsBuffer->Map(0, nullptr, &pShapeWeightsBuffer);
        memcpy(pShapeWeightsBuffer, humanoidMesh.shapeWeights.data(), shapeWeightsSize);
        humanoidMesh.pMesh->shapeWeightsBuffer->Unmap(0, nullptr);

        //�V�F�C�v�L�[�̈ʒu����ێ����郊�\�[�X���쐬
        CreateShapeDeltasTexture(humanoidMesh);
    }
}

void Character::CreateShapeDeltasTexture(HumanoidMesh& humanoidMesh)
{
    //GPU�o�b�t�@�iStructuredBuffer�j�̃��\�[�X���쐬
    size_t bufferSize = sizeof(XMFLOAT3) * humanoidMesh.shapeWeights.size() * humanoidMesh.vertexCount; //�K�v�ȃo�b�t�@�T�C�Y���v�Z
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    //CPU����̕ύX���󂯕t���Ȃ��V�F�C�v�L�[�p�̃��\�[�X���쐬
    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    HRESULT hr = m_pDevice->CreateCommittedResource(&defaultHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, 
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&humanoidMesh.pMesh->shapeDeltasBuffer));
    if (FAILED(hr)) {
        printf("shapeDeltasBuffer�̍쐬�Ɏ��s���܂����B\n");
    }

    //�ꎞ�A�b�v���[�h�o�b�t�@���쐬
    ComPtr<ID3D12Resource> uploadBuffer;
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    hr = m_pDevice->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
    if (FAILED(hr) || !uploadBuffer) {
        printf("uploadBuffer�̍쐬�Ɏ��s���܂����B\n");
    }

    //�ꎞ�o�b�t�@�Ƀf�[�^���R�s�[
    void* mappedData;
    uploadBuffer->Map(0, nullptr, &mappedData);
    memcpy(mappedData, humanoidMesh.shapeDeltas.data(), bufferSize);
    uploadBuffer->Unmap(0, nullptr);

    //�R�}���h���X�g�ɃR�s�[�R�}���h�𑗂� (�A�b�v���[�h�o�b�t�@��shapeDeltasBuffer�ɃR�s�[)
    m_pCommandList->CopyResource(humanoidMesh.pMesh->shapeDeltasBuffer.Get(), uploadBuffer.Get());

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        humanoidMesh.pMesh->shapeDeltasBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
    );
    m_pCommandList->ResourceBarrier(1, &barrier);

    //�R�}���h���C����p�����f�[�^�̓]���͈�x�����_�[�L���[���I�������Ȃ���΂Ȃ�Ȃ��B (���̂��ߓǂݍ��ލہA���t���[���`��ɒx����������\������)
    g_Engine->EndRender();
    g_Engine->BeginRender();

    Sleep(2);

    //��x�ݒ肵����ύX���Ȃ����߃N���A
    //humanoidMesh.shapeDeltas.clear();
}

void Character::CalculateBoneTransforms(const aiNode* node, const XMMATRIX& parentTransform)
{
    //�{�[����boneMap�ɑ��݂��邩�`�F�b�N
    auto it = m_boneManager.m_finalBoneTransforms.find(node->mName.C_Str());
    XMMATRIX nodeTransform = XMMatrixIdentity();

    if (it != m_boneManager.m_finalBoneTransforms.end()) {
        //�{�[���̃��[�J���ϊ��s����擾
        nodeTransform = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&node->mTransformation)));
        //printf("NodeName = %s, x=%f, y=%f, z=%f\n", node->mName.C_Str(), nodeTransform.r[3].m128_f32[0], nodeTransform.r[3].m128_f32[1], nodeTransform.r[3].m128_f32[2]);
    }

    //�e�{�[���̕ϊ��s��Ƃ̍����i�e����q�ւ̕ϊ��j
    XMMATRIX globalTransform = nodeTransform * parentTransform;

    //�{�[���̃��[���h��Ԃł̕ϊ��s���ۑ��iSphere��u�����߂̈ʒu�Ƃ��Ďg�p�j
    if (it != m_boneManager.m_finalBoneTransforms.end()) {
        it->second = globalTransform;
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

            //���_���猩�āA�{�[�������݂���ʒu(�I�t�Z�b�g)���擾
            XMMATRIX boneOffset = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<XMFLOAT4X4*>(&bone->mOffsetMatrix)));

            m_boneManager.m_finalBoneTransforms[boneName] = XMMatrixIdentity();

            //�{�[�����쐬
            Bone boneChild(boneName, boneOffset);

            //��⑫�͍��E�𔻒� (��U��̔�������A���̏ꍇ��LoadBoneFamily���ŕύX)
            if (boneName[0] == 'L' || boneName.at(boneName.size() - 1) == 'L') {
                boneChild.m_bType = BONETYPE_LEFT_ARM;
            }
            if (boneName[0] == 'R' || boneName.at(boneName.size() - 1) == 'R') {
                boneChild.m_bType = BONETYPE_RIGHT_ARM;
            }

            //�ڂ̃{�[���͍��E�̋�ʂ����Ȃ�
            if (boneName.find("Eye") != std::string::npos || boneName.find("eye") != std::string::npos) {
                boneChild.m_bType = BONETYPE_DEFAULT;
            }

            //�z��ɒǉ�
            m_boneManager.m_boneInfos.push_back(XMMatrixIdentity());
            m_boneManager.m_bones.push_back(boneChild);

            //�{�[�����ƃC���f�b�N�X��R�Â�
            m_boneManager.m_boneMapping[bone->mName.C_Str()] = boneIndex;
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
        //�{�[����
        char* boneNameBuf = boneReader.ReadBytes(boneReader.ReadByte());
        std::string boneName = boneNameBuf;
        delete[] boneNameBuf;

        //�{�[���̏����ʒu
        XMMATRIX matrix = boneReader.ReadMatrix();

        Bone bone(boneName, matrix);
        //�{�[���^�C�v
        bone.m_bType = (BoneType)boneReader.ReadSByte();

        m_boneManager.m_finalBoneTransforms[boneName] = boneReader.ReadMatrix();

        //�{�[������Index��R�Â�
        m_boneManager.m_boneMapping[boneName] = static_cast<UINT>(m_boneManager.m_bones.size());
        //�z��ɒǉ�
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

//�{�[���̐e�q�֌W���擾
void Character::LoadBoneFamily(const aiNode* node)
{
    if (m_boneManager.Exist(node->mName.C_Str())) {
        UINT boneIndex = m_boneManager.m_boneMapping[node->mName.C_Str()];
        for (UINT i = 0; i < node->mNumChildren; i++) {
            if (m_boneManager.m_boneMapping.find(node->mChildren[i]->mName.C_Str()) != m_boneManager.m_boneMapping.end()) {
                UINT childIndex = m_boneManager.m_boneMapping[node->mChildren[i]->mName.C_Str()];

                //�q�{�[���Ɛe�{�[����ݒ�
                m_boneManager.m_bones[boneIndex].AddChildBone(&m_boneManager.m_bones[childIndex], childIndex);
                m_boneManager.m_bones[childIndex].SetParentBone(&m_boneManager.m_bones[boneIndex], boneIndex);

                std::string boneName = m_boneManager.m_bones[childIndex].GetBoneName();
                bool bExchanged = false;
                if (boneName.find("Leg") != std::string::npos || boneName.find("leg") != std::string::npos) {
                    if (m_boneManager.m_bones[childIndex].m_bType == BONETYPE_LEFT_ARM) {
                        m_boneManager.m_bones[childIndex].m_bType = BONETYPE_LEFT_LEG;
                    }
                    else if (m_boneManager.m_bones[childIndex].m_bType == BONETYPE_RIGHT_ARM) {
                        m_boneManager.m_bones[childIndex].m_bType = BONETYPE_RIGHT_LEG;
                    }
                    bExchanged = true;
                }
                if (m_boneManager.m_bones[boneIndex].GetBoneName().find("shoulder") != std::string::npos) {
                    bExchanged = true;
                }

                if (!bExchanged && m_boneManager.m_bones[boneIndex].m_bType != BONETYPE_DEFAULT) {
                    m_boneManager.m_bones[childIndex].m_bType = m_boneManager.m_bones[boneIndex].m_bType;
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
                //printf("Shape = %d\n", shapeIndex);
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
    //���k����Ă���V�F�C�v������
    std::vector<char> humanoidBuffer;
    BinaryDecompress(humanoidBuffer, humanoidBufferOriginalSize, compressedBuffer, humanoidBufferCompressedSize);

    UINT endTime = timeGetTime();
    printf("Decompress - %dms\n", endTime - startTime);

    delete[] compressedBuffer;

    //�𓀂����o�b�t�@��ǂݍ���
    BinaryReader humanoidReader(humanoidBuffer);
    UINT humanoidMeshCount = humanoidReader.ReadUInt32();

    for (UINT i = 0; i < humanoidMeshCount; i++) {
        HumanoidMesh humanoidMesh;

        //���b�V����
        char* shapeNameBuf = humanoidReader.ReadBytes(humanoidReader.ReadByte());
        humanoidMesh.meshName = shapeNameBuf;
        delete[] shapeNameBuf;

        UINT shapeMappingCount = humanoidReader.ReadUInt32();
        for (UINT j = 0; j < shapeMappingCount; j++) {
            //�V�F�C�v�L�[��
            char* shapeNameBuf = humanoidReader.ReadBytes(humanoidReader.ReadByte());
            std::string shapeName = shapeNameBuf;
            delete[] shapeNameBuf;

            //�V�F�C�v�L�[�̃C���f�b�N�X
            UINT shapeIndex = humanoidReader.ReadUInt32();

            humanoidMesh.shapeWeights.push_back(0.0f);
            humanoidMesh.shapeMapping[shapeName] = shapeIndex;

            /*if (humanoidMesh.meshName == "Body all") {
                printf("%u - %s\n", j, shapeName.c_str());
            }*/
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
}

void Character::SetTexture(const std::string nameOnly)
{
    std::string dir = "Resource\\Model\\";

    std::string texPath = dir + nameOnly;
    std::string normalPath = dir + "Skin_Normal Map.png";

    //�e�N�X�`�����쐬
    Texture2D* mainTex = Texture2D::Get(texPath);
    Texture2D* normalMap = Texture2D::Get(normalPath);
    //�}�e���A�����쐬
    m_pDescriptorHeap->SetMainTexture(mainTex->Resource(), normalMap->Resource(), m_pShadowMapBuffer);
    m_textures.push_back(mainTex);
    m_textures.push_back(normalMap);
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
        //printf("shapeName = %s, Index = %u\n", shapeName.c_str(), shapeIndex);
        mesh.shapeWeights[shapeIndex] = weight;
    }
}

void Character::UpdateBoneTransform(UINT boneIndex, XMMATRIX& parentMatrix)
{
    XMMATRIX parentTransform = XMMatrixIdentity();

    //�e�{�[�������݂���ꍇ�A���̃��[���h���W��K�p
    if (m_boneManager.m_bones[boneIndex].GetParentBone()) {
        parentTransform = m_boneManager.m_bones[boneIndex].GetParentBone()->GetGlobalTransform();
    }
    else {
        Bone& armatureBone = m_boneManager.m_armatureBone;
        XMMATRIX armatureScale = XMMatrixScaling(armatureBone.m_scale.x, armatureBone.m_scale.y, armatureBone.m_scale.z);
        XMVECTOR armatureRotVec = XMVectorSet(-armatureBone.m_rotation.x, -armatureBone.m_rotation.z, -armatureBone.m_rotation.y, armatureBone.m_rotation.w);
        XMMATRIX armatureRot = XMMatrixRotationQuaternion(armatureRotVec);

        XMMATRIX armaturePos = XMMatrixTranslation(-armatureBone.m_position.x, -armatureBone.m_position.z, armatureBone.m_position.y);

        XMMATRIX boneTransform = armatureScale * armatureRot * armaturePos;

        //�e�̃��[���h�ϊ��ƃ��[�J���ϊ�������
        parentTransform = XMMatrixTranspose(boneTransform);
    }

    //�{�[���̍ŏI�I�Ȉʒu�A��]�A�X�P�[��������

    Bone& bone = m_boneManager.m_bones[boneIndex];

    XMMATRIX scale = XMMatrixScaling(bone.m_scale.x, bone.m_scale.y, bone.m_scale.z);
    XMVECTOR rotVec = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

    //Unity�Ƃ̍��W�n�̈Ⴂ���C�� (�����o���̂ɂ߂��������᎞�Ԃ��������B�Ȃ�Ŏ�Ƒ��Ɠ��̂Ŏd�l�Ⴄ�́B)
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

    //printf("%s -> x=%f, y=%f, z=%f, w=%f\n", bone.GetBoneName().c_str(), XMVectorGetX(rotVec), XMVectorGetY(rotVec), XMVectorGetZ(rotVec), XMVectorGetW(rotVec));

    XMMATRIX rot = XMMatrixRotationQuaternion(rotVec);

    XMMATRIX pos = XMMatrixTranslation(bone.m_position.x, bone.m_position.z, bone.m_position.y);
    XMMATRIX offsetBack2 = m_boneManager.m_finalBoneTransforms[bone.GetBoneName()];
    XMMATRIX offsetBack = XMMatrixIdentity();

    //�ʒu�̂ݒ��o
    offsetBack.r[3].m128_f32[0] = offsetBack2.r[3].m128_f32[0];
    offsetBack.r[3].m128_f32[1] = offsetBack2.r[3].m128_f32[1];
    offsetBack.r[3].m128_f32[2] = offsetBack2.r[3].m128_f32[2];

    //XMMATRIX boneTransform = scale * offsetBack * rot * bone.GetBoneOffset() * pos;
    XMMATRIX boneTransform = scale * XMMatrixInverse(nullptr, offsetBack) * rot * offsetBack * pos;

    //�e�̃��[���h�ϊ��ƃ��[�J���ϊ�������
    XMMATRIX finalTransform = parentTransform * XMMatrixTranspose(boneTransform);

    //�V�F�[�_�[�ɑ��郏�[���h���W�ɑ��
    m_boneManager.m_boneInfos[boneIndex] = finalTransform;
    bone.SetGlobalTransform(finalTransform);

    //�q�{�[���ɂ��ϊ���`�d
    for (UINT i = 0; i < bone.GetChildBoneCount(); i++) {
        UpdateBoneTransform(bone.GetChildBoneIndex(i), m_boneManager.m_boneInfos[boneIndex]);
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
        memcpy(pData, m_boneManager.m_boneInfos.data(), sizeof(XMMATRIX) * m_boneManager.m_boneInfos.size()); //�{�[���}�g���b�N�X���R�s�[
        m_boneMatricesBuffer->Unmap(0, nullptr);
    }
}

void Character::UpdateShapeKeys()
{
    for (HumanoidMesh& mesh : m_humanoidMeshes) {
        //�V�F�C�v�o�b�t�@�ɑ��M
        if (mesh.pMesh->shapeWeightsBuffer) {
            void* pData = nullptr;
            HRESULT hr = mesh.pMesh->shapeWeightsBuffer->Map(0, nullptr, &pData);
            if (SUCCEEDED(hr))
            {
                memcpy(pData, mesh.shapeWeights.data(), sizeof(float) * mesh.shapeWeights.size()); //�e�V�F�C�v�L�[�̃E�F�C�g���R�s�[
                mesh.pMesh->shapeWeightsBuffer->Unmap(0, nullptr);
            }
        }
    }
}

void Character::UpdateAnimation()
{
    //�A�j���[�V���������݂��Ȃ�
    if (m_nowAnimationIndex < 0 || m_animations.size() <= m_nowAnimationIndex) {
        return;
    }

    //�L�[�t���[�������݂��Ȃ�
    if (m_animations[m_nowAnimationIndex].m_frames.size() <= 0) {
        return;
    }

    //�A�j���[�V�������Ԃ��X�V
    m_nowAnimationTime += g_Engine->GetFrameTime() * m_animationSpeed;

    //���݂̃A�j���[�V�������Ԃ̃t���[�����擾
    AnimationFrame* pFrame = m_animations[m_nowAnimationIndex].GetFrame(m_nowAnimationTime);

    //�t���[�������݂��Ȃ���Ώ������I�� (��ɃA�j���[�V�������ǂݍ��܂�Ă��Ȃ��ꍇ)
    if (!pFrame) {
        return;
    }

    m_boneManager.m_armatureBone.m_position = pFrame->armatureAnimation.position;
    m_boneManager.m_armatureBone.m_rotation = pFrame->armatureAnimation.rotation;

    //�{�[���A�j���[�V����
    for (UINT i = 0; i < pFrame->boneAnimations.size(); i++) {
        std::string boneName = m_animations[m_nowAnimationIndex].m_boneMapping[i];
        UpdateBonePosition(boneName, pFrame->boneAnimations[i].position);
        UpdateBoneRotation(boneName, pFrame->boneAnimations[i].rotation);
    }

    //�V�F�C�v�L�[�̃A�j���[�V����
    for (UINT i = 0; i < m_animations[m_nowAnimationIndex].m_shapeNames.size(); i++) {
        std::string& shapeName = m_animations[m_nowAnimationIndex].m_shapeNames[i];
        SetShapeWeight(shapeName, pFrame->shapeAnimations[i]);
    }

    //�Đ����̃t���[�����Ō�̃t���[���������ꍇ�ŏ��ɖ߂�
    if (m_animations[m_nowAnimationIndex].IsLastFrame(pFrame)) {
        m_nowAnimationTime = 0.0f;
    }
}

Character::HumanoidMesh::HumanoidMesh()
    : pMesh(nullptr)
    , vertexCount(0)
{
}
