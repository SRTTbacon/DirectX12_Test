#include "Model.h"
#include <d3dx12.h>
#include <stdexcept>
#include "..\\..\\Main\\Main.h"

Model::Model(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const Camera* pCamera, DirectionalLight* pDirectionalLight, ID3D12Resource* pShadowMapBuffer)
    : m_modelMatrix(XMMatrixIdentity())
    , m_pCamera(pCamera)
    , m_pDevice(pDevice)
    , m_pCommandList(pCommandList)
    , m_pDirectionalLight(pDirectionalLight)
    , m_pShadowMapBuffer(pShadowMapBuffer)
    , m_pPipelineState(nullptr)
    , m_pRootSignature(nullptr)
    , m_pDescriptorHeap(nullptr)
    , m_position(XMFLOAT3(0.0f, 0.0f, 0.0f))
    , m_rotation(XMFLOAT3(0.0f, 0.0f, 0.0f))
    , m_scale(XMFLOAT3(1.0f, 1.0f, 1.0f))
    , m_depth(0.0f)
    , m_bVisible(true)
    , m_bTransparent(false)
{
}

void Model::LoadModel(const std::string fbxFile)
{
    m_fbxFile = fbxFile;

    CreateConstantBuffer();

    Assimp::Importer importer;

    //���f���ǂݍ��ݎ��̃t���O�B���b�V���̃|���S���͂��ׂĎO�p�`�ɂ��A������W�n�ɕϊ�
    unsigned int flag = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
        aiProcess_CalcTangentSpace;
    const aiScene* scene = importer.ReadFile(fbxFile, flag);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        printf("FBX�t�@�C�������[�h�ł��܂���ł����B\n");
        return;
    }

    //�f�B�X�N���v�^�q�[�v���쐬 (���b�V���̐� + �e1��)
    m_pDescriptorHeap = new DescriptorHeap(m_pDevice, scene->mNumMeshes * 2, ShadowSizeHigh);

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

        vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
        vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
        if (mesh->mTextureCoords[0]) {
            vertex.texCoords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        }
        else {
            vertex.texCoords = { 0.0f, 0.0f };
        }
        vertex.boneIDs[0] = vertex.boneIDs[1] = vertex.boneIDs[2] = vertex.boneIDs[3] = 0;
        vertex.boneWeights = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);

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

    //meshData->materialIndex = -1;

    //�}�e���A�����쐬
    m_pDescriptorHeap->SetMainTexture(Texture2D::GetWhite()->Resource(), m_pShadowMapBuffer);

    return meshData;
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
        else {
            printf("�R���X�^���g�o�b�t�@�𐶐����܂����B%s\n", m_fbxFile.c_str());
        }
    }

    //�f�B���N�V���i�����C�g�̏��
    CD3DX12_RESOURCE_DESC lightBuf = CD3DX12_RESOURCE_DESC::Buffer(sizeof(LightBuffer));
    m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &lightBuf,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_lightConstantBuffer));
}

void Model::RenderShadowMap(UINT backBufferIndex)
{
    m_pCommandList->SetGraphicsRootConstantBufferView(0, m_modelConstantBuffer[backBufferIndex]->GetGPUVirtualAddress());

    if (m_boneMatricesBuffer) {
        m_pCommandList->SetGraphicsRootConstantBufferView(1, m_boneMatricesBuffer->GetGPUVirtualAddress());   //�{�[���𑗐M
    }

    //���b�V���̐[�x�����V���h�E�}�b�v�ɕ`��
    for (const Mesh* pMesh : m_meshes)
    {
        pMesh->Draw(m_pCommandList); //�[�x�݂̂�`��
    }
}

void Model::RenderSceneWithShadow(UINT backBufferIndex)
{
    //�R�}���h���X�g�ɑ��M
    m_pCommandList->SetGraphicsRootSignature(m_pRootSignature->GetRootSignature());   //���[�g�V�O�l�`����ݒ�
    m_pCommandList->SetPipelineState(m_pPipelineState->GetPipelineState());           //�p�C�v���C���X�e�[�g��ݒ�

    m_pCommandList->SetGraphicsRootConstantBufferView(0, m_modelConstantBuffer[backBufferIndex]->GetGPUVirtualAddress()); //���f���̈ʒu�֌W�𑗐M
    m_pCommandList->SetGraphicsRootConstantBufferView(1, m_lightConstantBuffer->GetGPUVirtualAddress()); //�f�B���N�V���i�����C�g�̏��𑗐M

    if (m_boneMatricesBuffer) {
        m_pCommandList->SetGraphicsRootConstantBufferView(3, m_boneMatricesBuffer->GetGPUVirtualAddress());   //�{�[���𑗐M
    }

    // �f�B�X�N���v�^�q�[�v��ݒ肵�A�V���h�E�}�b�v���T���v�����O�ł���悤�ɂ���
    ID3D12DescriptorHeap* heaps[] = { m_pDescriptorHeap->GetHeap() };
    m_pCommandList->SetDescriptorHeaps(1, heaps);

    // �J�������_����V�[����`�� (�V���h�E���v�Z���A�J���[�o��)
    for (size_t i = 0; i < m_meshes.size(); i++)
    {
        m_pCommandList->SetGraphicsRootDescriptorTable(2, m_pDescriptorHeap->GetGpuDescriptorHandle((int)i));

        Mesh* pMesh = m_meshes[i];

        if (pMesh->contentsBuffer) {
            m_pCommandList->SetGraphicsRootConstantBufferView(4, pMesh->contentsBuffer->GetGPUVirtualAddress());      //���_���𑗐M
        }

        if (pMesh->shapeDeltasBuffer) {
            m_pCommandList->SetGraphicsRootShaderResourceView(5, pMesh->shapeDeltasBuffer->GetGPUVirtualAddress());   //�V�F�C�v�L�[�̈ʒu���𑗐M
        }

        if (pMesh->shapeWeightsBuffer) {
            m_pCommandList->SetGraphicsRootShaderResourceView(6, pMesh->shapeWeightsBuffer->GetGPUVirtualAddress());  //�V�F�C�v�L�[�̃E�F�C�g���𑗐M
        }

        m_meshes[i]->Draw(m_pCommandList); // �J���[�ƃV���h�E���v�Z���`��
    }
}

void Model::Update(UINT backBufferIndex)
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
    mcb.projectionMatrix = XMMatrixPerspectiveFovRH(m_pCamera->m_fov, m_pCamera->m_aspect, m_pCamera->m_near, m_pCamera->m_far);
    mcb.lightViewProjMatrix = m_pDirectionalLight->lightViewProj;
    mcb.normalMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, mcb.modelMatrix));

    //�萔�o�b�t�@�Ƀf�[�^����������
    void* p0;
    HRESULT hr = m_modelConstantBuffer[backBufferIndex]->Map(0, nullptr, &p0);
    if (p0) {
        memcpy(p0, &mcb, sizeof(ModelConstantBuffer));
        m_modelConstantBuffer[backBufferIndex]->Unmap(0, nullptr);
    }
    if (FAILED(hr)) {
        printf("�o�b�t�@�̍X�V�Ɏ��s���܂����B�G���[�R�[�h:%d\n", hr);

        HRESULT reason = m_pDevice->GetDeviceRemovedReason();
        if (reason != S_OK) {
            // �f�o�C�X�폜�̌��������O�ɏo��
            printf("Device removed reason: 0x%08X\n", reason);
        }
    }

    void* p1;
    m_lightConstantBuffer->Map(0, nullptr, &p1);
    if (p1) {
        memcpy(p1, &m_pDirectionalLight->lightBuffer, sizeof(LightBuffer));
        m_lightConstantBuffer->Unmap(0, nullptr);
    }

    XMVECTOR objPos = XMLoadFloat3(&m_position);
    XMVECTOR camPos = m_pCamera->m_eyePos;
    m_depth = XMVectorGetZ(XMVector3Length(objPos - camPos)); //Z������[�x�Ƃ��Ď擾

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
{
}

void Model::Mesh::Draw(ID3D12GraphicsCommandList* pCommandList) const
{
    pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);           //���_���𑗐M
    pCommandList->IASetIndexBuffer(&indexBufferView);                    //�C���f�b�N�X���𑗐M
    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);  //3�p�|���S���̂�

    pCommandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);  //�`��
}
