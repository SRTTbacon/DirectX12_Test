#include "Character.h"

Character::Character(const std::string fbxFile, const Camera* pCamera)
	: Model(pCamera)
    , m_animationSpeed(0.77f)
    , m_nowAnimationTime(0.0f)
{
	LoadFBX(fbxFile);

	m_pRootSignature = new RootSignature(m_pDevice, ShaderKinds::BoneShader);
	m_pPipelineState = new PipelineState(m_pDevice, m_pRootSignature);

	printf("���f��:%s�����[�h���܂����B\n", fbxFile.c_str());
}

void Character::LoadAnimation(std::string animFile)
{
    m_pAnimation = g_Engine->GetAnimation(animFile);
}

void Character::Update()
{
    //�{�[���̃}�g���b�N�X���X�V
    UpdateBoneTransform();

    //�V�F�C�v�L�[�̃E�F�C�g���X�V
    UpdateShapeKeys();

    //�A�j���[�V�����̍X�V
    UpdateAnimation();

    Model::Update();
}

void Character::LoadFBX(const std::string& fbxFile)
{
    Assimp::Importer importer;

    //���f���ǂݍ��ݎ��̃t���O�B���b�V���̃|���S���͂��ׂĎO�p�`�ɂ��A�{�[�������݂���ꍇ�͉e�����󂯂�E�F�C�g��4�܂łɂ���
    UINT flag = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
        aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights;// | aiProcess_MakeLeftHanded;
    const aiScene* scene = importer.ReadFile(fbxFile, flag);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        printf("FBX�t�@�C�������[�h�ł��܂���ł����B\n");
        return;
    }

    ProcessNode(scene, scene->mRootNode);

    //�{�[���ɐe�q�֌W��t����
    LoadBoneFamily(scene->mRootNode);

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
                //�}�e���A���C���f�b�N�X�͍Ō�ɒǉ��������̂��g�p
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
    //���b�V��������
    for (UINT i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        HumanoidMesh humanoidMesh{};
        Mesh* pMesh = ProcessMesh(scene, mesh, humanoidMesh);
        m_meshes.push_back(pMesh);
        m_humanoidMeshes.push_back(humanoidMesh);
    }

    // �q�m�[�h���ċA�I�ɏ���
    for (UINT i = 0; i < node->mNumChildren; i++) {
        ProcessNode(scene, node->mChildren[i]);
    }
}

Model::Mesh* Character::ProcessMesh(const aiScene* scene, aiMesh* mesh, HumanoidMesh& humanoidMesh) {
    std::vector<Vertex> vertices;
    std::vector<UINT> indices;

    //���_�̏���
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

    //�C���f�b�N�X�̏���
    for (UINT i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (UINT j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    //���_����ۑ�
    humanoidMesh.vertexCount = static_cast<UINT>(vertices.size());

    //���_�o�b�t�@�̍쐬
    Mesh* meshData = new Mesh();
    humanoidMesh.pMesh = meshData;

    //�{�[���̏���
    LoadBones(scene, mesh, vertices);

    //�V�F�C�v�L�[�̏���
    LoadShapeKey(mesh, vertices, humanoidMesh);

    //�V�F�[�_�[�ɕK�v�ȃo�b�t�@�𐶐�
    CreateBuffer(meshData, vertices, indices, humanoidMesh);

    return meshData;
}

void Character::CreateBuffer(Mesh* pMesh, std::vector<Vertex>& vertices, std::vector<UINT>& indices, HumanoidMesh& humanoidMesh)
{
    //���_�o�b�t�@��ݒ�
    const UINT vertexBufferSize = static_cast<UINT>(sizeof(Vertex) * vertices.size());

    //�q�[�v�ݒ�
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    //���_�o�b�t�@�̃��\�[�X
    CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    //�f�o�C�X�ō쐬
    HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pMesh->vertexBuffer));

    if (FAILED(hr)) {
        printf("���_�o�b�t�@�̐����Ɏ��s���܂����B%1xl\n", hr);
    }

    //���_�f�[�^��GPU�ɑ��M
    void* vertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    pMesh->vertexBuffer->Map(0, &readRange, &vertexDataBegin);
    memcpy(vertexDataBegin, vertices.data(), vertexBufferSize);
    pMesh->vertexBuffer->Unmap(0, nullptr);

    pMesh->vertexBufferView.BufferLocation = pMesh->vertexBuffer->GetGPUVirtualAddress();
    pMesh->vertexBufferView.StrideInBytes = sizeof(Vertex);
    pMesh->vertexBufferView.SizeInBytes = vertexBufferSize;

    //�C���f�b�N�X�o�b�t�@�̍쐬
    const UINT indexBufferSize = static_cast<UINT>(sizeof(UINT) * indices.size());

    CD3DX12_RESOURCE_DESC indexBuffer = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

    hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &indexBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pMesh->indexBuffer));
    if (FAILED(hr)) {
        printf("�C���f�b�N�X�o�b�t�@�̐����Ɏ��s���܂����B\n");
    }

    //�C���f�b�N�X�f�[�^��GPU�������ɋL�^
    void* indexDataBegin;
    pMesh->indexBuffer->Map(0, &readRange, &indexDataBegin);
    memcpy(indexDataBegin, indices.data(), indexBufferSize);
    pMesh->indexBuffer->Unmap(0, nullptr);

    pMesh->indexBufferView.BufferLocation = pMesh->indexBuffer->GetGPUVirtualAddress();
    pMesh->indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    pMesh->indexBufferView.SizeInBytes = indexBufferSize;

    pMesh->indexCount = static_cast<UINT>(indices.size());

    //�{�[�����̃��\�[�X���쐬
    CD3DX12_RESOURCE_DESC boneBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(XMMATRIX) * m_boneInfos.size());
    hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &boneBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_boneMatricesBuffer));
    if (FAILED(hr)) {
        printf("�{�[���o�b�t�@�̐����Ɏ��s���܂����B\n");
    }

    //���_����ێ�����p�̃��\�[�X���쐬
    CD3DX12_RESOURCE_DESC contentsBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Contents));
    hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &contentsBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pMesh->contentsBuffer));
    if (FAILED(hr)) {
        printf("�R���e���c�o�b�t�@�̐����Ɏ��s���܂����B\n");
    }

    Contents contents{};
    contents.vertexCount = static_cast<UINT>(vertices.size());
    contents.shapeCount = static_cast<UINT>(humanoidMesh.shapeWeights.size());
    void* pContentsBuffer;
    pMesh->contentsBuffer->Map(0, nullptr, &pContentsBuffer);
    memcpy(pContentsBuffer, &contents, sizeof(Contents));
    pMesh->contentsBuffer->Unmap(0, nullptr);

    //�V�F�C�v�L�[�̏���ێ�����p�̃��\�[�X���쐬
    if (humanoidMesh.shapeWeights.size() > 0) {
        //�V�F�C�v�L�[�̃E�F�C�g����ێ����郊�\�[�X���쐬
        UINT64 shapeWeightsSize = sizeof(float) * humanoidMesh.shapeWeights.size();
        D3D12_RESOURCE_DESC shapeWeightsBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(shapeWeightsSize);
        hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &shapeWeightsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pMesh->shapeWeightsBuffer));
        if (FAILED(hr)) {
            printf("�V�F�C�v�L�[�E�F�C�g�o�b�t�@�̐����Ɏ��s���܂����B%ld\n", hr);
        }

        //�E�F�C�g���̏����l������ (Update�֐��ŏ�ɍX�V�����)
        void* pShapeWeightsBuffer;
        pMesh->shapeWeightsBuffer->Map(0, nullptr, &pShapeWeightsBuffer);
        memcpy(pShapeWeightsBuffer, humanoidMesh.shapeWeights.data(), shapeWeightsSize);
        pMesh->shapeWeightsBuffer->Unmap(0, nullptr);

        //�V�F�C�v�L�[�̈ʒu����ێ����郊�\�[�X���쐬
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
    //GPU�o�b�t�@�iStructuredBuffer�j�̃��\�[�X���쐬
    size_t bufferSize = sizeof(XMFLOAT3) * humanoidMesh.shapeWeights.size() * humanoidMesh.vertexCount; //�K�v�ȃo�b�t�@�T�C�Y���v�Z
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

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
    g_Engine->CommandList()->CopyResource(humanoidMesh.pMesh->shapeDeltasBuffer.Get(), uploadBuffer.Get());

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        humanoidMesh.pMesh->shapeDeltasBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
    );
    g_Engine->CommandList()->ResourceBarrier(1, &barrier);

    //�R�}���h���C����p�����f�[�^�̓]���͈�x�����_�[�L���[���I�������Ȃ���΂Ȃ�Ȃ��B (���̂��ߓǂݍ��ލہA���t���[���`��ɒx����������\������)
    g_Engine->EndRender();
    g_Engine->BeginRender();

    //��x�ݒ肵����ύX���Ȃ����߃N���A
    humanoidMesh.shapeDeltas.clear();
}

static XMVECTOR ExtractEulerAngles(const XMMATRIX& matrix) {
    float pitch, yaw, roll;

    // Y�������̉�]�iyaw�j
    yaw = asinf(-matrix.r[2].m128_f32[0]);

    if (cosf(yaw) > 0.0001f) {
        // X����Z�������̉�]�ipitch �� roll�j
        pitch = atan2f(matrix.r[2].m128_f32[1], matrix.r[2].m128_f32[2]);
        roll = atan2f(matrix.r[1].m128_f32[0], matrix.r[0].m128_f32[0]);
    }
    else {
        // Y���� 90�x�܂��� -90�x�̎��́AGimbal Lock�ɑΉ�
        pitch = atan2f(-matrix.r[0].m128_f32[2], matrix.r[1].m128_f32[1]);
        roll = 0.0f;
    }

    return XMVectorSet(pitch, yaw, roll, 0.0f); // ���W�A���p�ŏo��
}

void Character::LoadBones(const aiScene* scene, aiMesh* mesh, std::vector<Vertex>& vertices)
{
    for (UINT i = 0; i < mesh->mNumBones; i++) {
        aiBone* bone = mesh->mBones[i];
        UINT boneIndex = 0;

        // �{�[�����܂��o�^����Ă��Ȃ��ꍇ�A�}�b�s���O��ǉ�
        if (m_boneMapping.find(bone->mName.C_Str()) == m_boneMapping.end()) {
            //�ǉ�����{�[���̃C���f�b�N�X���擾
            boneIndex = static_cast<UINT>(m_boneInfos.size());

            //���_���猩�āA�{�[�������݂���ʒu(�I�t�Z�b�g)���擾
            XMMATRIX boneOffset = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<XMFLOAT4X4*>(&bone->mOffsetMatrix)));

            //�{�[�����쐬
            Bone boneChild(bone->mName.C_Str(), boneOffset);

            XMMATRIX& m = boneChild.GetBoneOffset();

            XMVECTOR eulerAngles = ExtractEulerAngles(m);
            // �e���̉�]�p�i���W�A���j
            float pitch = XMVectorGetX(eulerAngles); // X���̉�]�i�s�b�`�j
            float yaw = XMVectorGetY(eulerAngles);   // Y���̉�]�i���[�j
            float roll = XMVectorGetZ(eulerAngles);  // Z���̉�]�i���[���j

            if (abs(yaw) < 1.0) {
                //Y����Z��������  (�f�t�H���g��Z���������̂���)
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

            //�Ȃ����I�t�Z�b�g�̃v���X�}�C�i�X�����]���Ă��܂��{�[�������邽�߁A����(Y��)�̂ݏ�Ƀv���X�֏C��
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

            //�z��ɒǉ�
            m_boneInfos.push_back(XMMatrixIdentity());
            m_bones.push_back(boneChild);

            //�{�[�����ƃC���f�b�N�X��R�Â�
            m_boneMapping[bone->mName.C_Str()] = boneIndex;
        }
        else {
            //�����̃{�[�������݂���ꍇ������g�p (�قȂ郁�b�V���Ԃœ����̃{�[���̏ꍇ�A�قƂ�ǂ����L����Ă��邽��)
            boneIndex = m_boneMapping[bone->mName.C_Str()];
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
                // 4�ȏ�̃{�[���E�F�C�g�̓T�|�[�g�O
                //printf("���_ %d �� 4 �ȏ�̃{�[���E�F�C�g�������Ă��܂��B\n", vertexID);
            }
        }
    }
}

//�{�[���̐e�q�֌W���擾
void Character::LoadBoneFamily(const aiNode* node)
{
    if (m_boneMapping.find(node->mName.C_Str()) != m_boneMapping.end()) {
        UINT boneIndex = m_boneMapping[node->mName.C_Str()];
        for (UINT i = 0; i < node->mNumChildren; i++) {
            if (m_boneMapping.find(node->mChildren[i]->mName.C_Str()) != m_boneMapping.end()) {
                UINT childIndex = m_boneMapping[node->mChildren[i]->mName.C_Str()];

                //�q�{�[���Ɛe�{�[����ݒ�
                m_bones[boneIndex].AddChildBone(&m_bones[childIndex], childIndex);
                m_bones[childIndex].SetParentBone(boneIndex);
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
        //
        if (humanoidMesh.shapeMapping.find(shapeName) == humanoidMesh.shapeMapping.end()) {
            UINT shapeIndex = static_cast<UINT>(humanoidMesh.shapeWeights.size());

            humanoidMesh.shapeWeights.push_back(0.0f);
            humanoidMesh.shapeMapping[shapeName] = shapeIndex;

            printf("shapeName = %s, Index = %d\n", shapeName.c_str(), shapeIndex);

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

//�{�[���̈ʒu���X�V
void Character::UpdateBonePosition(std::string boneName, XMFLOAT3& position)
{
    if (m_boneMapping.find(boneName) == m_boneMapping.end()) {
        return;
    }

    UINT boneIndex = m_boneMapping[boneName];

    m_bones[boneIndex].m_position = position;
}

//�{�[���̉�]��ύX
void Character::UpdateBoneRotation(std::string boneName, XMFLOAT4& rotation)
{
    if (m_boneMapping.find(boneName) == m_boneMapping.end()) {
        return;
    }

    UINT boneIndex = m_boneMapping[boneName];

    m_bones[boneIndex].m_rotation = rotation;
}

//�{�[���̃X�P�[����ύX
void Character::UpdateBoneScale(std::string boneName, XMFLOAT3& scale)
{
    if (m_boneMapping.find(boneName) == m_boneMapping.end()) {
        return;
    }

    UINT boneIndex = m_boneMapping[boneName];

    m_bones[boneIndex].m_scale = scale;
}

//�V�F�C�v�L�[�̃E�F�C�g���X�V
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

//���ׂẴ{�[�������擾
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

    //�e�{�[�������݂���ꍇ�A���̃��[���h���W��K�p
    if (m_bones[boneIndex].GetParentBoneIndex() != UINT32_MAX) {
        parentTransform = m_boneInfos[m_bones[boneIndex].GetParentBoneIndex()];
    }

    //�{�[���̍ŏI�I�Ȉʒu�A��]�A�X�P�[��������

    Bone& bone = m_bones[boneIndex];
    XMMATRIX scale = XMMatrixScaling(bone.m_scale.x, bone.m_scale.y, bone.m_scale.z);

    //XMMATRIX rotX = XMMatrixRotationX(XMConvertToRadians(bone.m_rotation.x));  //X��
    //XMMATRIX rotY = XMMatrixRotationY(XMConvertToRadians(bone.m_rotation.z));  //Unity��ɂ��邽�߁AY����Z�������ւ�
    //XMMATRIX rotZ = XMMatrixRotationZ(XMConvertToRadians(bone.m_rotation.y));  //Unity��ɂ��邽�߁AY����Z�������ւ�
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

    //�e�̃��[���h�ϊ��ƃ��[�J���ϊ�������
    XMMATRIX finalTransform = parentTransform * XMMatrixTranspose(boneTransform);

    //�V�F�[�_�[�ɑ��郏�[���h���W�ɑ��
    m_boneInfos[boneIndex] = finalTransform;

    // �q�{�[���ɂ��ϊ���`�d
    for (UINT i = 0; i < bone.GetChildBoneCount(); i++) {
        UpdateBoneTransform(bone.GetChildBoneIndex(i), m_boneInfos[boneIndex]);
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
        memcpy(pData, m_boneInfos.data(), sizeof(XMMATRIX) * m_boneInfos.size()); // �{�[���}�g���b�N�X���R�s�[
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
                memcpy(pData, mesh.shapeWeights.data(), sizeof(float) * mesh.shapeWeights.size()); // �e�V�F�C�v�L�[�̃E�F�C�g���R�s�[
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

    //�Đ����̃t���[�����Ō�̃t���[���������ꍇ�ŏ��ɖ߂�
    if (m_pAnimation->IsLastFrame(pFrame)) {
        m_nowAnimationTime = 0.0f;
    }
}
