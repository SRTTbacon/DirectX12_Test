#include "Model.h"
#include <d3dx12.h>
#include <stdexcept>
#include "..\\..\\Main\\Main.h"

//------Mesh------

Mesh::Mesh()
    : pMaterial(nullptr)
    , pShapeWeightsMapped(nullptr)
    , pModel(nullptr)
    , bDraw(true)
    , bDrawShadow(true)
{
}

Mesh::~Mesh()
{
}

void Mesh::DrawMesh(ID3D12GraphicsCommandList* pCommandList, UINT backBufferIndex, XMMATRIX& modelMatrix, bool bShadowMode)
{
    //�`��t���O�������Ă��Ȃ��ꍇ�͕`�悵�Ȃ�
    if (!bDraw || !bDrawShadow) {
        return;
    }

    MeshCostantBuffer mcb{};

    //�萔�o�b�t�@�Ƀf�[�^����������
    XMMATRIX scale = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
    XMMATRIX rotX = XMMatrixRotationX(m_rotation.x);
    XMMATRIX rotY = XMMatrixRotationY(m_rotation.y);
    XMMATRIX rotZ = XMMatrixRotationZ(m_rotation.z);
    XMMATRIX rot = rotX * rotY * rotZ;
    XMMATRIX pos = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
    mcb.meshMatrix = scale * rot * pos;
    XMMATRIX worldMatrix = mcb.meshMatrix * modelMatrix;
    mcb.normalMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix));
    mcb.time = pModel->GetModelTime();

    pCommandList->SetGraphicsRootConstantBufferView(0, pModel->GetConstantBuffer(backBufferIndex)->GetGPUVirtualAddress()); //���f���̈ʒu�֌W�𑗐M
    pCommandList->SetGraphicsRoot32BitConstants(1, sizeof(MeshCostantBuffer) / 4, &mcb, 0);

    if (bShadowMode) {
        if (meshData->contentsBuffer) {
            pCommandList->SetGraphicsRootConstantBufferView(3, meshData->contentsBuffer->GetGPUVirtualAddress());      //���_���𑗐M
        }

        if (meshData->shapeDeltasBuffer) {
            pCommandList->SetGraphicsRootDescriptorTable(4, pMaterial->GetShapeData(meshData->shapeDataIndex));        //���_�V�F�[�_�[�̃V�F�C�v�L�[
        }

        if (shapeWeightsBuffer) {
            pCommandList->SetGraphicsRootShaderResourceView(5, shapeWeightsBuffer->GetGPUVirtualAddress());  //�V�F�C�v�L�[�̃E�F�C�g���𑗐M
        }
    }
    else {
        if (pModel->GetBoneBuffer()) {
            pCommandList->SetGraphicsRootConstantBufferView(6, pModel->GetBoneBuffer()->GetGPUVirtualAddress());   //�{�[���𑗐M
        }

        if (pModel->GetModelType() == ModelType_Character) {   //�{�[�������݂���V�F�[�_�[�̂�
            if (meshData->contentsBuffer) {
                pCommandList->SetGraphicsRootConstantBufferView(7, meshData->contentsBuffer->GetGPUVirtualAddress());      //���_���𑗐M
            }

            if (shapeWeightsBuffer) {
                pCommandList->SetGraphicsRootShaderResourceView(8, shapeWeightsBuffer->GetGPUVirtualAddress());  //�V�F�C�v�L�[�̃E�F�C�g���𑗐M
            }
        }
    }

    pCommandList->IASetVertexBuffers(0, 1, &meshData->vertexBufferView);            //���_���𑗐M
    pCommandList->IASetIndexBuffer(&meshData->indexBufferView);                     //�C���f�b�N�X���𑗐M
    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);      //3�p�|���S���̂�

    pCommandList->DrawIndexedInstanced(meshData->indexCount, 1, 0, 0, 0);  //�`��
}

//------Model------

std::unordered_map<std::string, Model::MeshDataList> Model::s_sharedMeshes;

Model::Model(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const Camera* pCamera, const DirectionalLight* pDirectionalLight, MaterialManager* pMaterialManager, float* pFrameTime)
    : m_modelType(ModelType_Primitive)
    , m_pCamera(pCamera)
    , m_pDirectionalLight(pDirectionalLight)
    , m_pDevice(pDevice)
    , m_pCommandList(pCommandList)
    , m_pMaterialManager(pMaterialManager)
    , m_pFrameTime(pFrameTime)
    , m_modelConstantStruct(ModelConstantBuffer())
    , m_depth(0.0f)
    , m_animationSpeed(1.0f)
    , m_nowAnimationTime(0.0f)
    , m_modelTime(0.0f)
    , m_bShadowRendered(false)
    , m_bDrawShadow(true)
    , m_pBoneMatricesMap(nullptr)
    , m_nowAnimationIndex(-1)
{
    for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++) {
        m_pMappedConstantBuffer[i] = nullptr;
    }
}

Model::~Model()
{
	for (Mesh* pMesh : m_meshes) {
		delete pMesh;
	}
	m_meshes.clear();
    if (s_sharedMeshes.find(m_modelFile) != s_sharedMeshes.end()) {
        s_sharedMeshes[m_modelFile].Release();
        if (s_sharedMeshes[m_modelFile].refCount <= 0) {
            s_sharedMeshes.erase(m_modelFile);
        }
    }
}

void Model::LoadModel(const std::string modelFile)
{
    m_modelFile = modelFile;

    DWORD startTime = timeGetTime();
    CreateConstantBuffer();

    if (SetSharedMeshes()) {
        DWORD endTime = timeGetTime();
        //printf("LoadModel - %s MeshCount - %u : %dms\n", modelFile.c_str(), static_cast<UINT>(m_meshes.size()), endTime - startTime);
        return;
    }

    std::string extension = GetFileExtension(modelFile);

    DWORD tempTime = timeGetTime();
    printf("CreateConstantBuffer - %dms\n", tempTime - startTime);

    BinaryReader br(modelFile, 1);
    char* headerBuf = br.ReadBytes(br.ReadByte());
    std::string header = headerBuf;
    delete[] headerBuf;
    br.Close();

    DWORD tempTime2 = timeGetTime();
    printf("Open - %dms\n", tempTime2 - tempTime);
    if (header == std::string(MODEL_HEADER)) {
        //�o�C�i���t�@�C���Ƃ��ĊJ��
        BinaryReader br(modelFile);

        //�w�b�_�[
        char* headerBuf = br.ReadBytes(br.ReadByte());

        delete[] headerBuf;

        //���f���̃t�H�[�}�b�g
        BYTE format = br.ReadByte();
        //�L�����N�^�[�ȊO�͏������Ȃ�
        if (format != MODEL_DEFAULT) {
            br.Close();
            return;
        }

        UINT meshCount = br.ReadUInt32();
        for (UINT i = 0; i < meshCount; i++) {
            Mesh* pMesh = ProcessMesh(br, i);
            m_meshes.push_back(pMesh);
        }
        tempTime = timeGetTime();
        printf("ProcessMesh - %dms\n", tempTime - tempTime2);

        ProcessAnimation(br);

        tempTime2 = timeGetTime();
        printf("ProcessMesh - %dms\n", tempTime2 - tempTime);
    }
    else {
        //ReadFile���Ɠ��{��̃p�X�������Ă���Ɠǂݍ��߂Ȃ����߁Aifstream�œǂݍ���œn��
        std::ifstream file(modelFile, std::ios::binary);
        if (!file) {
            printf("���f���t�@�C�����J���܂���: %s\n", modelFile.c_str());
            return;
        }

        std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        Assimp::Importer importer;

        //���f���ǂݍ��ݎ��̃t���O�B���b�V���̃|���S���͂��ׂĎO�p�`�ɂ��A������W�n�ɕϊ�
        unsigned int flag = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace | aiProcess_RemoveRedundantMaterials | aiProcess_GenUVCoords | aiProcess_OptimizeMeshes;
        const aiScene* scene = importer.ReadFileFromMemory(buffer.data(), buffer.size(), flag, extension.c_str());

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            printf("FBX�t�@�C�������[�h�ł��܂���ł����B%s\n", importer.GetErrorString());
            return;
        }

        ProcessNode(scene, scene->mRootNode);

        ProcessAnimation(scene);
    }

    DWORD endTime = timeGetTime();
    printf("LoadModel - %s MeshCount - %u : %dms\n", modelFile.c_str(), static_cast<UINT>(m_meshes.size()), endTime - startTime);
}

void Model::Update()
{
    UpdateAnimation();
}

void Model::ProcessNode(const aiScene* pScene, aiNode* pNode) {
    //�m�[�h�Ƀ��b�V�����܂܂�Ă�����ǂݍ���
    for (UINT j = 0; j < pNode->mNumMeshes; j++) {
        aiMesh* pAiMesh = pScene->mMeshes[pNode->mMeshes[j]];
        UINT meshIndex = static_cast<UINT>(m_meshes.size());
        Mesh* pMesh = ProcessMesh(pScene, pAiMesh, meshIndex);

        pMesh->meshName = UTF8ToShiftJIS(pNode->mName.C_Str());

        //�����ʒu�A�p�x���擾
        XMMATRIX matrix = GetMeshDefaultMatrix(pNode);
        pMesh->m_position = XMFLOAT3(matrix.r[3].m128_f32[0], matrix.r[3].m128_f32[1], matrix.r[3].m128_f32[2]) / 100.0f;
        XMVECTOR rotation = ExtractEulerAngles(matrix);
        pMesh->m_rotation = XMFLOAT3(XMVectorGetX(rotation), XMVectorGetY(rotation), XMVectorGetZ(rotation));

        MeshDataList::MeshDataMain& meshMain = s_sharedMeshes[m_modelFile].meshMain[meshIndex];
        meshMain.meshName = pMesh->meshName;
        meshMain.position = pMesh->m_position;
        meshMain.rotation = pMesh->m_rotation;

        m_meshes.push_back(pMesh);
    }

    for (UINT i = 0; i < pNode->mNumChildren; i++) {
        aiNode* childNode = pNode->mChildren[i];
        ProcessNode(pScene, childNode);
    }
}

Mesh* Model::ProcessMesh(const aiScene* scene, aiMesh* mesh, UINT meshIndex) {
    //���ɓ������f�������[�h����Ă���΁A������Q��
    if (s_sharedMeshes.find(m_modelFile) != s_sharedMeshes.end() && s_sharedMeshes[m_modelFile].meshDataList.size() > meshIndex) {
        MeshDataList& meshDataList = s_sharedMeshes[m_modelFile];

        std::vector<VertexPrimitive> vertex;
        std::vector<UINT> index;
        std::shared_ptr<MeshData> meshData = meshDataList.meshDataList[meshIndex];

        Mesh* pMesh = new Mesh();

        //�e�N�X�`���̓f�t�H���g
        pMesh->pMaterial = m_pMaterialManager->AddMaterial("PrimitiveWhite");
        pMesh->pModel = this;
        pMesh->meshName = meshDataList.meshMain[meshIndex].meshName;
        pMesh->m_position = meshDataList.meshMain[meshIndex].position;
        pMesh->m_rotation = meshDataList.meshMain[meshIndex].rotation;

        CreateBuffer(pMesh, vertex, index, sizeof(VertexPrimitive), meshIndex);

        return pMesh;
    }

    std::vector<VertexPrimitive> vertices;
    std::vector<UINT> indices;

    //���_�̏���
    for (UINT i = 0; i < mesh->mNumVertices; i++) {
        VertexPrimitive vertex{};

        vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
		vertex.boneWeights = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);                              //�{�[���E�F�C�g�͎g��Ȃ�
		vertex.boneIDs[0] = vertex.boneIDs[1] = vertex.boneIDs[2] = vertex.boneIDs[3] = 0;  //�{�[��ID�͎g��Ȃ�
        vertex.vertexID = i;
        vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
        if (mesh->mTextureCoords[0]) {
            vertex.texCoords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        }
        else {
            vertex.texCoords = { 0.0f, 0.0f };
        }
        vertex.tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
        vertex.bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };

        vertices.push_back(vertex);
    }

    //�C���f�b�N�X�̏���
    for (UINT i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (UINT j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    Mesh* meshData = new Mesh();

    CreateBuffer(meshData, vertices, indices, sizeof(VertexPrimitive), meshIndex);

    //�e�N�X�`���̓f�t�H���g
    meshData->pMaterial = m_pMaterialManager->AddMaterial("PrimitiveWhite");
    meshData->pModel = this;

    return meshData;
}

Mesh* Model::ProcessMesh(BinaryReader& br, UINT meshIndex)
{
    DWORD tempTime1 = timeGetTime();

    UINT meshBufferOriginalSize = br.ReadUInt32();
    UINT meshBufferCompressedSize = br.ReadUInt32();
    char* compressedBuffer = br.ReadBytes(meshBufferCompressedSize);

    DWORD tempTime3 = timeGetTime();
    //printf("Model Read - %dms\n", tempTime3 - tempTime1);

    //���k����Ă���{�[��������
    std::vector<char> meshBuffer;
    BinaryDecompress(meshBuffer, meshBufferOriginalSize, compressedBuffer, meshBufferCompressedSize);

    delete[] compressedBuffer;

    BinaryReader meshReader(meshBuffer);

    DWORD tempTime2 = timeGetTime();
    //printf("Model Decompress - %dms\n", tempTime2 - tempTime1);

    //�𓀂����o�b�t�@��ǂݍ���
    std::vector<VertexPrimitive> vertices;
    std::vector<UINT> indices;

    //���_�̏���
    UINT vertexCount = meshReader.ReadUInt32();
    for (UINT i = 0; i < vertexCount; i++) {
        VertexPrimitive vertex{};

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

    tempTime1 = timeGetTime();
    //printf("Model Vertex - %dms\n", tempTime1 - tempTime2);

    //�C���f�b�N�X�̏���
    UINT faceCount = meshReader.ReadUInt32();
    for (UINT i = 0; i < faceCount; i++) {
        UINT indexCount = meshReader.ReadUInt32();
        for (UINT j = 0; j < indexCount; j++) {
            indices.push_back(meshReader.ReadUInt32());
        }
    }

    Mesh* meshData = new Mesh();

    char* pMeshName = meshReader.ReadBytes(meshReader.ReadByte());
    meshData->meshName = pMeshName;
    delete pMeshName;

    meshData->m_position = meshReader.ReadFloat3();
    meshData->m_rotation = meshReader.ReadFloat3();

    //�V�F�[�_�[�ɕK�v�ȃo�b�t�@�𐶐�
    Model::CreateBuffer(meshData, vertices, indices, sizeof(VertexPrimitive), meshIndex);

    MeshDataList& meshDataList = s_sharedMeshes[m_modelFile];
    meshDataList.meshMain[meshIndex].meshName = meshData->meshName;
    meshDataList.meshMain[meshIndex].position = meshData->m_position;
    meshDataList.meshMain[meshIndex].rotation = meshData->m_rotation;

    //�e�N�X�`���̓f�t�H���g
    meshData->pMaterial = m_pMaterialManager->AddMaterial("PrimitiveWhite");
    meshData->pModel = this;

    tempTime2 = timeGetTime();
    //printf("Model Index - %dms\n", tempTime2 - tempTime1);

    return meshData;
}

void Model::ProcessAnimation(const aiScene* pScene)
{
    for (UINT i = 0; i < pScene->mNumAnimations; i++) {
        aiAnimation* pAnim = pScene->mAnimations[i];

        //�A�j���[�V�������Ԃ��v�Z
        double ticksPerSecond = (pAnim->mTicksPerSecond != 0) ? pAnim->mTicksPerSecond : 30.0;
        double durationInSeconds = pAnim->mDuration / ticksPerSecond;

        Animation animation;
        animation.SetMaxTime(static_cast<float>(durationInSeconds));

        for (UINT j = 0; j < pAnim->mNumChannels; j++) {
            aiNodeAnim* pChannel = pAnim->mChannels[j];
            UINT meshIndex = UINT_MAX;

            std::string meshName = UTF8ToShiftJIS(pChannel->mNodeName.C_Str());

            for (UINT k = 0; k < m_meshes.size(); k++) {
                if (m_meshes[k]->meshName == meshName) {
                    meshIndex = k;
                    break;
                }
            }
            if (meshIndex == UINT_MAX) {
                continue;
            }

            for (UINT k = 0; k < pChannel->mNumPositionKeys; k++) {
                XMFLOAT3 value = XMFLOAT3(pChannel->mPositionKeys[k].mValue.x, pChannel->mPositionKeys[k].mValue.y, pChannel->mPositionKeys[k].mValue.z);
                float timeInSeconds = static_cast<float>(pChannel->mPositionKeys[k].mTime / ticksPerSecond);
                animation.AddFrame(Animation::FrameType::FrameType_Position, meshIndex, timeInSeconds, value);
            }
            for (UINT k = 0; k < pChannel->mNumRotationKeys; k++) {
                aiQuaternion& q = pChannel->mRotationKeys[k].mValue;
                //�N�I�[�^�j�I���̐���
                double x = q.x;
                double y = q.y;
                double z = q.z;
                double w = q.w;

                //Pitch (X����])
                double sinPitch = 2.0 * (w * x + y * z);
                double cosPitch = 1.0 - 2.0 * (x * x + y * y);
                float pitch = static_cast<float>(std::atan2(sinPitch, cosPitch));

                //Yaw (Y����])
                double sinYaw = 2.0 * (w * y - z * x);
                sinYaw = std::clamp(sinYaw, -1.0, 1.0); // asin�͈̔͊O�΍�
                float yaw = static_cast<float>(std::asin(sinYaw));

                //Roll (Z����])
                double sinRoll = 2.0 * (w * z + x * y);
                double cosRoll = 1.0 - 2.0 * (y * y + z * z);
                float roll = static_cast<float>(std::atan2(sinRoll, cosRoll));

                XMFLOAT3 value = XMFLOAT3(pitch, yaw, roll);
                float timeInSeconds = static_cast<float>(pChannel->mRotationKeys[k].mTime / ticksPerSecond);
                animation.AddFrame(Animation::FrameType::FrameType_Rotation, meshIndex, timeInSeconds, value);
            }
            for (UINT k = 0; k < pChannel->mNumScalingKeys; k++) {
                XMFLOAT3 value = XMFLOAT3(pChannel->mScalingKeys[k].mValue.x, pChannel->mScalingKeys[k].mValue.y, pChannel->mScalingKeys[k].mValue.z);
                float timeInSeconds = static_cast<float>(pChannel->mScalingKeys[k].mTime / ticksPerSecond);
                animation.AddFrame(Animation::FrameType::FrameType_Scale, meshIndex, timeInSeconds, value);
            }
        }

        m_animations.push_back(animation);

        s_sharedMeshes[m_modelFile].animations.push_back(animation);

        m_nowAnimationIndex = 0;
    }
}

void Model::ProcessAnimation(BinaryReader& br)
{
    UINT animBufferOriginalSize = br.ReadUInt32();
    UINT animBufferCompressedSize = br.ReadUInt32();
    char* compressedBuffer = br.ReadBytes(animBufferCompressedSize);

    //���k����Ă���{�[��������
    std::vector<char> animBuffer;
    BinaryDecompress(animBuffer, animBufferOriginalSize, compressedBuffer, animBufferCompressedSize);

    delete[] compressedBuffer;

    BinaryReader animReader(animBuffer);

    UINT animationCount = animReader.ReadUInt32();

    for (UINT i = 0; i < animationCount; i++) {
        Animation animation{};

        animReader.ReadFloat();     //1�b�����`�b�N��
        float animationTime = animReader.ReadFloat();

        animation.SetMaxTime(animationTime);

        UINT channelCount = animReader.ReadUInt32();

        for (UINT j = 0; j < channelCount; j++) {
            //�{�[��������index���擾
            char* meshNameBuf = animReader.ReadBytes(animReader.ReadByte());
            std::string meshName = meshNameBuf;
            delete[] meshNameBuf;

            UINT meshIndex = UINT_MAX;

            for (UINT k = 0; k < m_meshes.size(); k++) {
                if (m_meshes[k]->meshName == meshName) {
                    meshIndex = k;
                    break;
                }
            }
            if (meshIndex == UINT_MAX) {
                continue;
            }

            UINT positionCount = animReader.ReadUInt32();
            UINT rotationCount = animReader.ReadUInt32();
            UINT scaleCount = animReader.ReadUInt32();

            for (UINT k = 0; k < positionCount; k++) {
                float time = animReader.ReadFloat();
                XMFLOAT3 value = animReader.ReadFloat3();
                animation.AddFrame(Animation::FrameType::FrameType_Position, meshIndex, time, value);
            }

            for (UINT k = 0; k < rotationCount; k++) {
                float time = animReader.ReadFloat();
                XMFLOAT3 value = animReader.ReadFloat3();
                animation.AddFrame(Animation::FrameType::FrameType_Rotation, meshIndex, time, value);
            }

            for (UINT k = 0; k < scaleCount; k++) {
                float time = animReader.ReadFloat();
                XMFLOAT3 value = animReader.ReadFloat3();
                animation.AddFrame(Animation::FrameType::FrameType_Scale, meshIndex, time, value);
            }
        }

        m_animations.push_back(animation);

        m_nowAnimationIndex = 0;
    }
}

void Model::UpdateAnimation()
{
    //�A�j���[�V���������݂��Ȃ�
    if (m_nowAnimationIndex < 0 || m_animations.size() <= m_nowAnimationIndex) {
        return;
    }

    //�L�[�t���[�������݂��Ȃ�
    if (m_animations[m_nowAnimationIndex].GetMaxTime() <= 0.0f) {
        return;
    }

    //�A�j���[�V�������Ԃ��X�V
    m_nowAnimationTime += *m_pFrameTime * m_animationSpeed;

    //���݂̃A�j���[�V�������Ԃ̃t���[�����擾
    ModelAnimationFrame frame = m_animations[m_nowAnimationIndex].GetModelFrame(m_nowAnimationTime);
    for (std::pair<UINT, XMFLOAT3> pair : frame.position) {
        m_meshes[pair.first]->m_position = pair.second;
    }
    for (std::pair<UINT, XMFLOAT3> pair : frame.rotation) {
        m_meshes[pair.first]->m_rotation = pair.second;
    }
    for (std::pair<UINT, XMFLOAT3> pair : frame.scale) {
        m_meshes[pair.first]->m_scale = pair.second;
    }

    //�Đ����̃t���[�����Ō�̃t���[���������ꍇ�ŏ��ɖ߂�
    if (m_animations[m_nowAnimationIndex].IsLastFrame(m_nowAnimationTime)) {
        m_nowAnimationTime = 0.0f;
    }
}

bool Model::SetSharedMeshes()
{
    //���f�����܂����[�h����Ă��Ȃ���Ώ������I����
    if (s_sharedMeshes.find(m_modelFile) == s_sharedMeshes.end()) {
        return false;
    }

    //���ɓǂݍ��ݍς݂̃��f��
    //���b�V�������R�s�[
    for (size_t i = 0; i < s_sharedMeshes[m_modelFile].meshDataList.size(); i++) {
        MeshDataList& meshDataList = s_sharedMeshes[m_modelFile];

        std::vector<VertexPrimitive> vertex;
        std::vector<UINT> index;
        std::shared_ptr<MeshData> meshData = meshDataList.meshDataList[i];

        Mesh* pMesh = new Mesh();

        //�e�N�X�`���̓f�t�H���g
        pMesh->pMaterial = m_pMaterialManager->AddMaterial("PrimitiveWhite");
        pMesh->pModel = this;
        pMesh->meshName = meshDataList.meshMain[i].meshName;
        pMesh->m_position = meshDataList.meshMain[i].position;
        pMesh->m_rotation = meshDataList.meshMain[i].rotation;

        CreateBuffer(pMesh, vertex, index, sizeof(VertexPrimitive), static_cast<UINT>(i));

        m_meshes.push_back(pMesh);
    }

    //�A�j���[�V�������R�s�[
    MeshDataList& meshDataList = s_sharedMeshes[m_modelFile];
    if (meshDataList.animations.size() > 0) {
        m_animations = meshDataList.animations;
        m_nowAnimationIndex = 0;
    }

    return true;
}

XMMATRIX Model::GetMeshDefaultMatrix(aiNode* pNode)
{
    XMMATRIX parentMatrix = XMMatrixIdentity();
    if (pNode->mParent != nullptr) {
        parentMatrix = GetMeshDefaultMatrix(pNode->mParent);
    }

    XMMATRIX currentMatrix = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&pNode->mTransformation)));
    return currentMatrix * parentMatrix;
}

void Model::CreateConstantBuffer()
{
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    CD3DX12_RESOURCE_DESC constantData = CD3DX12_RESOURCE_DESC::Buffer((sizeof(ModelConstantBuffer) + 255) & ~255);
    //�萔�o�b�t�@�����\�[�X�Ƃ��č쐬
    for (int i = 0; i < FRAME_BUFFER_COUNT; i++) {
        HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &constantData,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_modelConstantBuffer[i]));

        if (FAILED(hr)) {
            printf("�R���X�^���g�o�b�t�@�̐����Ɏ��s:�G���[�R�[�h%d\n", hr);
        }

        m_modelConstantBuffer[i]->Map(0, nullptr, &m_pMappedConstantBuffer[i]);
    }
}

void Model::RenderShadowMap(UINT backBufferIndex)
{
    if (!m_bVisible || m_bShadowRendered || !m_bDrawShadow) {
        return;
    }

    m_bShadowRendered = true;

    if (m_boneMatricesBuffer) {
        m_pCommandList->SetGraphicsRootConstantBufferView(2, m_boneMatricesBuffer->GetGPUVirtualAddress());   //�{�[���𑗐M
    }

    //���b�V���̐[�x�����V���h�E�}�b�v�ɕ`��
    for (size_t i = 0; i < m_meshes.size(); i++)
    {
        if (!m_meshes[i]->bDraw) {
            continue;
        }

        Mesh* pMesh = m_meshes[i];
        pMesh->DrawMesh(m_pCommandList, backBufferIndex, m_modelConstantStruct.modelMatrix, true); //�[�x�݂̂�`��
    }
}

void Model::LateUpdate(UINT backBufferIndex)
{
    if (!m_bVisible) {
        return;
    }

    m_bShadowRendered = false;

    //�ʒu�A��]�A�X�P�[��������
    ModelConstantBuffer& mcb = m_modelConstantStruct;
    XMMATRIX scale = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
    XMMATRIX rotX = XMMatrixRotationX(m_rotation.x);
    XMMATRIX rotY = XMMatrixRotationY(m_rotation.y);
    XMMATRIX rotZ = XMMatrixRotationZ(m_rotation.z);
    XMMATRIX rot = rotX * rotY * rotZ;
    XMMATRIX pos = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
    mcb.modelMatrix = scale * rot * pos;
    mcb.viewMatrix = m_pCamera->m_viewMatrix;
    mcb.projectionMatrix = m_pCamera->m_projMatrix;
    mcb.lightViewProjMatrix = m_pDirectionalLight->m_lightViewProj;
    mcb.cameraPos.x = m_pCamera->m_eyePos.m128_f32[0];
    mcb.cameraPos.y = m_pCamera->m_eyePos.m128_f32[0];
    mcb.cameraPos.z = m_pCamera->m_eyePos.m128_f32[0];
    mcb.cameraPos.w = 1.0f;
	const XMFLOAT3& lightPos = m_pDirectionalLight->m_position;
	mcb.lightPos.x = lightPos.x;
	mcb.lightPos.y = lightPos.y;
	mcb.lightPos.z = lightPos.z;
	mcb.lightPos.w = 1.0f;

    m_modelTime += *m_pFrameTime;

    memcpy(m_pMappedConstantBuffer[backBufferIndex], &mcb, sizeof(ModelConstantBuffer));

    //�J�����ʒu����I�u�W�F�N�g�ʒu�܂ł̋�����m_depth�ɐݒ�
    XMFLOAT3 camPos;
    XMStoreFloat3(&camPos, m_pCamera->m_eyePos);
    m_depth = DistanceSq(m_position, camPos);
}
