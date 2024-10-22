#include "Model.h"
#include <d3dx12.h>
#include <stdexcept>
#include "..\\..\\Main\\Main.h"

ID3D12Resource* modelConstantBuffer;

Model::Model(const Camera* pCamera)
    : m_modelMatrix(XMMatrixIdentity())
    , m_pCamera(pCamera)
    , m_position(XMFLOAT3(0.0f, 0.0f, 0.0f))
    , m_rotation(XMFLOAT3(0.0f, 0.0f, 0.0f))
    , m_scale(XMFLOAT3(1.0f, 1.0f, 1.0f))
    , m_pDescriptorHeap(nullptr)
    , m_pPipelineState(nullptr)
    , m_pRootSignature(nullptr)
{
    m_pDevice = g_Engine->Device();

    auto eyePos = XMVectorSet(0.0f, 5.0f, 1.0f, 0.0f); // ���_�̈ʒu
    auto targetPos = XMVectorSet(0.0f, 0.0f, 0.75f, 0.0f); // ���_����������W
    auto upward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // �������\���x�N�g��
    constexpr auto fov = XMConvertToRadians(37.5); // ����p
    auto aspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT); // �A�X�y�N�g��

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    CD3DX12_RESOURCE_DESC constantData = CD3DX12_RESOURCE_DESC::Buffer(sizeof(ModelConstantBuffer));
    //�萔�o�b�t�@�����\�[�X�Ƃ��č쐬
    for (int i = 0; i < FRAME_BUFFER_COUNT; i++) {
        m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &constantData,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_modelConstantBuffer[i]));
    }
}

void Model::LoadModel(const PrimitiveModel primitiveModel)
{
    if (primitiveModel == PrimitiveModel::Primitive_Sphere) {
        LoadSphere(0.2f, 32, 32, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    m_pRootSignature = new RootSignature(m_pDevice, ShaderKinds::PrimitiveShader);
    m_pPipelineState = new PipelineState(m_pDevice, m_pRootSignature);

    printf("���b�V���𐶐����܂����B\n");
}

void Model::LoadModel(const std::string fbxFile)
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

    m_pRootSignature = new RootSignature(m_pDevice, ShaderKinds::PrimitiveShader);
    m_pPipelineState = new PipelineState(m_pDevice, m_pRootSignature);
}

void Model::ProcessNode(const aiScene* scene, aiNode* node) {
    // ���b�V��������
    for (UINT i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_meshes.push_back(ProcessMesh(scene, mesh));
    }

    // �q�m�[�h���ċA�I�ɏ���
    for (UINT i = 0; i < node->mNumChildren; i++) {
        ProcessNode(scene, node->mChildren[i]);
    }
}

Model::Mesh* Model::ProcessMesh(const aiScene* scene, aiMesh* mesh) {
    std::vector<VertexPrimitive> vertices;
    std::vector<UINT> indices;

    // ���_�̏���
    for (UINT i = 0; i < mesh->mNumVertices; i++) {
        VertexPrimitive vertex{};

        vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
        vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
        vertex.Color = { 1.0f, 1.0f,1.0f,1.0f };
        if (mesh->mTextureCoords[0]) {
            vertex.TexCoords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        }
        else {
            vertex.TexCoords = { 0.0f, 0.0f };
        }

        vertices.push_back(vertex);
    }

    // �C���f�b�N�X�̏���
    for (UINT i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (UINT j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    Mesh* meshData = new Mesh();

    CreateBuffer(meshData, vertices, indices);

    meshData->materialIndex = -1;

    return meshData;
}

void Model::Update()
{
    //�ʒu�A��]�A�X�P�[��������
    ModelConstantBuffer mcb{};
    XMMATRIX scale = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
    XMMATRIX rotX = XMMatrixRotationX(XMConvertToRadians(m_rotation.x));
    XMMATRIX rotY = XMMatrixRotationY(XMConvertToRadians(m_rotation.y));
    XMMATRIX rotZ = XMMatrixRotationZ(XMConvertToRadians(m_rotation.z));
    XMMATRIX rot = rotX * rotY * rotZ;
    XMMATRIX pos = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
    mcb.modelMatrix = scale * rot * pos;
    mcb.viewMatrix = XMMatrixLookAtRH(m_pCamera->m_eyePos, m_pCamera->m_targetPos, m_pCamera->m_upFoward);
    mcb.projectionMatrix = XMMatrixPerspectiveFovRH(m_pCamera->m_fov, m_pCamera->m_aspect, 0.01f, 1000.0f);
    mcb.lightViewProjMatrix = g_Engine->GetDirectionalLight()->lightViewProj;
    mcb.lightDirection = g_Engine->GetDirectionalLight()->lightDirection;

    //�萔�o�b�t�@�Ƀf�[�^����������
    void* p;
    UINT bufferIndex = g_Engine->CurrentBackBufferIndex();
    m_modelConstantBuffer[bufferIndex]->Map(0, nullptr, &p);
    memcpy(p, &mcb, sizeof(ModelConstantBuffer));
    m_modelConstantBuffer[bufferIndex]->Unmap(0, nullptr);
}

void Model::Draw()
{
    //�G���W������R�}���h���X�g���擾
    ID3D12GraphicsCommandList* pCommandList = g_Engine->CommandList();

    //���݂̃o�b�N�o�b�t�@�̃C���f�b�N�X (�g���v���o�b�t�@�����O�̂��߁A0�`2���Ԃ����B�t���[���̕`�悪�I���Ύ��̃C���f�b�N�X�Ɉڍs)
    UINT bufferIndex = g_Engine->CurrentBackBufferIndex();

    //�R�}���h���X�g�ɑ��M
    pCommandList->SetGraphicsRootSignature(m_pRootSignature->GetRootSignature());                                   //���[�g�V�O�l�`����ݒ�
    pCommandList->SetPipelineState(m_pPipelineState->GetPipelineState());                                           //�p�C�v���C���X�e�[�g��ݒ�

    pCommandList->SetGraphicsRootConstantBufferView(0, m_modelConstantBuffer[bufferIndex]->GetGPUVirtualAddress()); //���f���̈ʒu�֌W�𑗐M

    if (m_boneMatricesBuffer) {
        pCommandList->SetGraphicsRootConstantBufferView(1, m_boneMatricesBuffer->GetGPUVirtualAddress());           //�{�[���𑗐M
    }

    if (m_pDescriptorHeap) {
        ID3D12DescriptorHeap* materialHeap = m_pDescriptorHeap->GetHeap();  //�f�B�X�N���v�^�q�[�v���擾
        pCommandList->SetDescriptorHeaps(1, &materialHeap);                 //�f�B�X�N���v�^�q�[�v�𑗐M
    }


    //���b�V���̐������J��Ԃ�
    for (size_t i = 0; i < m_meshes.size(); i++) {
        Mesh* pMesh = m_meshes[i];

        if (pMesh->contentsBuffer) {
            pCommandList->SetGraphicsRootConstantBufferView(2, pMesh->contentsBuffer->GetGPUVirtualAddress());      //���_���𑗐M
        }

        if (pMesh->shapeDeltasBuffer) {
            pCommandList->SetGraphicsRootShaderResourceView(3, pMesh->shapeDeltasBuffer->GetGPUVirtualAddress());   //�V�F�C�v�L�[�̈ʒu���𑗐M
        }

        if (pMesh->shapeWeightsBuffer) {
            pCommandList->SetGraphicsRootShaderResourceView(4, pMesh->shapeWeightsBuffer->GetGPUVirtualAddress());  //�V�F�C�v�L�[�̃E�F�C�g���𑗐M
        }

        pCommandList->IASetVertexBuffers(0, 1, &pMesh->vertexBufferView);           //���_���𑗐M
        pCommandList->IASetIndexBuffer(&pMesh->indexBufferView);                    //�C���f�b�N�X���𑗐M
        pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);  //3�p�|���S���̂�

        if (pMesh->materialIndex != -1)
            pCommandList->SetGraphicsRootDescriptorTable(5, g_materials[pMesh->materialIndex]->HandleGPU); //�}�e���A���𑗐M

        pCommandList->DrawIndexedInstanced(pMesh->indexCount, 1, 0, 0, 0);          //�`��
    }
}

void Model::LoadSphere(float radius, UINT sliceCount, UINT stackCount, const XMFLOAT4 color)
{
    Mesh* meshData = new Mesh();

    std::vector<VertexPrimitive> vertices;
    std::vector<UINT> indices;

    // ���_�̌v�Z
    VertexPrimitive topVertex = { XMFLOAT3(0.0f, +radius, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f), color };
    VertexPrimitive bottomVertex = { XMFLOAT3(0.0f, -radius, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f), color };

    vertices.push_back(topVertex);

    float phiStep = XM_PI / stackCount;
    float thetaStep = 2.0f * XM_PI / sliceCount;

    for (UINT i = 1; i < stackCount; ++i) {
        float phi = i * phiStep;

        for (UINT j = 0; j <= sliceCount; ++j) {
            float theta = j * thetaStep;

            VertexPrimitive v{};

            v.Position.x = radius * sinf(phi) * cosf(theta);
            v.Position.y = radius * cosf(phi);
            v.Position.z = radius * sinf(phi) * sinf(theta);

            v.Color = color;

            vertices.push_back(v);
        }
    }

    vertices.push_back(bottomVertex);

    // �C���f�b�N�X�̌v�Z
    for (uint32_t i = 1; i <= sliceCount; ++i) {
        indices.push_back(0);
        indices.push_back(i + 1);
        indices.push_back(i);
    }

    uint32_t baseIndex = 1;
    uint32_t ringVertexCount = sliceCount + 1;
    for (uint32_t i = 0; i < stackCount - 2; ++i) {
        for (uint32_t j = 0; j < sliceCount; ++j) {
            indices.push_back(baseIndex + i * ringVertexCount + j);
            indices.push_back(baseIndex + i * ringVertexCount + j + 1);
            indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);

            indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
            indices.push_back(baseIndex + i * ringVertexCount + j + 1);
            indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
        }
    }

    for (uint32_t i = 0; i < sliceCount; ++i) {
        indices.push_back((uint32_t)vertices.size() - 1);
        indices.push_back(baseIndex + (stackCount - 2) * ringVertexCount + i);
        indices.push_back(baseIndex + (stackCount - 2) * ringVertexCount + i + 1);
    }

    CreateBuffer(meshData, vertices, indices);
    meshData->materialIndex = -1;
}

void Model::CreateBuffer(Mesh* pMesh, std::vector<VertexPrimitive>& vertices, std::vector<UINT>& indices)
{
    //���_�o�b�t�@��ݒ�
    const UINT vertexBufferSize = static_cast<UINT>(sizeof(VertexPrimitive) * vertices.size());

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
        IID_PPV_ARGS(&pMesh->vertexBuffer)
    );

    if (FAILED(result)) {
        printf("���_�o�b�t�@�̐����Ɏ��s���܂����B\n");
    }


    //���_�f�[�^��GPU�ɑ��M
    void* vertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    pMesh->vertexBuffer->Map(0, &readRange, &vertexDataBegin);
    memcpy(vertexDataBegin, vertices.data(), vertexBufferSize);
    pMesh->vertexBuffer->Unmap(0, nullptr);

    pMesh->vertexBufferView.BufferLocation = pMesh->vertexBuffer->GetGPUVirtualAddress();
    pMesh->vertexBufferView.StrideInBytes = sizeof(VertexPrimitive);
    pMesh->vertexBufferView.SizeInBytes = vertexBufferSize;

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
        IID_PPV_ARGS(&pMesh->indexBuffer)
    );

    if (FAILED(result)) {
        printf("�C���f�b�N�X�o�b�t�@�̐����Ɏ��s���܂����B\n");
    }

    //�C���f�b�N�X�f�[�^��GPU�ɑ��M
    void* indexDataBegin;
    pMesh->indexBuffer->Map(0, &readRange, &indexDataBegin);
    memcpy(indexDataBegin, indices.data(), indexBufferSize);
    pMesh->indexBuffer->Unmap(0, nullptr);

    pMesh->indexBufferView.BufferLocation = pMesh->indexBuffer->GetGPUVirtualAddress();
    pMesh->indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    pMesh->indexBufferView.SizeInBytes = indexBufferSize;

    pMesh->indexCount = static_cast<UINT>(indices.size());
}

Model::Mesh::Mesh()
    : vertexBuffer(nullptr)
    , indexBuffer(nullptr)
    , contentsBuffer(nullptr)
    , shapeWeightsBuffer(nullptr)
    , shapeDeltasBuffer(nullptr)
    , vertexBufferView(D3D12_VERTEX_BUFFER_VIEW())
    , indexBufferView(D3D12_INDEX_BUFFER_VIEW())
    , indexCount(0)
    , materialIndex(-1)
{
}
