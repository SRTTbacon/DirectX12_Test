#include "Model.h"
#include <d3dx12.h>
#include <stdexcept>
#include "..\\..\\Main\\Main.h"

Model::Model(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const Camera* pCamera, const DirectionalLight* pDirectionalLight, MaterialManager* pMaterialManager)
    : m_modelMatrix(XMMatrixIdentity())
    , m_modelType(ModelType_Primitive)
    , m_pCamera(pCamera)
    , m_pDirectionalLight(pDirectionalLight)
    , m_pDevice(pDevice)
    , m_pCommandList(pCommandList)
    , m_pMaterialManager(pMaterialManager)
    , m_position(XMFLOAT3(0.0f, 0.0f, 0.0f))
    , m_rotation(XMFLOAT3(0.0f, 0.0f, 0.0f))
    , m_scale(XMFLOAT3(1.0f, 1.0f, 1.0f))
    , m_depth(0.0f)
    , m_bVisible(true)
    , m_bShadowRendered(false)
{
}

Model::~Model()
{
    /*
    //�e�N�X�`�������
    for (Texture2D* pTexture : m_textures) {
        delete pTexture;
    }
    m_textures.clear();

	for (Mesh* pMesh : m_meshes) {
		delete pMesh;
	}
	m_meshes.clear();

	if (m_pDescriptorHeap) {
		delete m_pDescriptorHeap;
        m_pDescriptorHeap = nullptr;
	}

	if (m_pPipelineState) {
		delete m_pPipelineState;
        m_pPipelineState = nullptr;
	}
	if (m_pRootSignature) {
		delete m_pRootSignature;
        m_pRootSignature = nullptr;
	}

	for (int i = 0; i < FRAME_BUFFER_COUNT; i++) {
		if (m_modelConstantBuffer[i]) {
			m_modelConstantBuffer[i]->Release();
		}
	}

    if (m_boneMatricesBuffer) {
        m_boneMatricesBuffer->Release();
    }
    if (m_shadowBoneMatricesBuffer) {
        m_shadowBoneMatricesBuffer->Release();
    }
    if (m_lightConstantBuffer) {
        m_lightConstantBuffer->Release();
    }
    */
}

void Model::LoadModel(const std::string fbxFile)
{
    m_modelFile = fbxFile;

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

    ProcessNode(scene, scene->mRootNode);
}

void Model::ProcessNode(const aiScene* pScene, aiNode* pNode) {
    //�m�[�h�Ƀ��b�V�����܂܂�Ă�����ǂݍ���
    for (UINT j = 0; j < pNode->mNumMeshes; j++) {
        aiMesh* pAiMesh = pScene->mMeshes[pNode->mMeshes[j]];
        Mesh* pMesh = ProcessMesh(pScene, pAiMesh);
        pMesh->meshName = UTF8ToShiftJIS(pNode->mName.C_Str());
        m_meshes.push_back(pMesh);
    }

    for (UINT i = 0; i < pNode->mNumChildren; i++) {
        aiNode* childNode = pNode->mChildren[i];
        ProcessNode(pScene, childNode);
    }
}

Mesh* Model::ProcessMesh(const aiScene* scene, aiMesh* mesh) {
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

    CreateBuffer(meshData, vertices, indices, sizeof(VertexPrimitive));

    //�e�N�X�`�������݂��Ȃ��ꍇ�͔��P�F�e�N�X�`�����g�p
    meshData->pMaterial = m_pMaterialManager->AddMaterial("PrimitiveWhite");
    meshData->pModel = this;

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
    }

    //�{�[�����̃��\�[�X���쐬 (�e�p�̃V�F�[�_�[)
    CD3DX12_RESOURCE_DESC boneBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(XMMATRIX) * MAX_BONE_COUNT);
    HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &boneBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_shadowBoneMatricesBuffer));
    if (FAILED(hr)) {
        printf("�{�[���o�b�t�@�̐����Ɏ��s���܂����B\n");
    }

    void* pBoneBuffer;
    std::vector<XMMATRIX> temp = std::vector<XMMATRIX>(MAX_BONE_COUNT, XMMatrixIdentity());
    m_shadowBoneMatricesBuffer->Map(0, nullptr, &pBoneBuffer);
    if (pBoneBuffer)
        memcpy(pBoneBuffer, temp.data(), sizeof(XMMATRIX) * MAX_BONE_COUNT);
    m_shadowBoneMatricesBuffer->Unmap(0, nullptr);
}

void Model::RenderShadowMap(UINT backBufferIndex)
{
    if (!m_bVisible || m_bShadowRendered) {
        return;
    }

    m_bShadowRendered = true;

    m_pCommandList->SetGraphicsRootConstantBufferView(0, m_modelConstantBuffer[backBufferIndex]->GetGPUVirtualAddress());

    if (m_boneMatricesBuffer) {
        m_pCommandList->SetGraphicsRootConstantBufferView(1, m_boneMatricesBuffer->GetGPUVirtualAddress());   //�{�[���𑗐M
    }

    //���b�V���̐[�x�����V���h�E�}�b�v�ɕ`��
    for (size_t i = 0; i < m_meshes.size(); i++)
    {
        if (!m_meshes[i]->bDraw) {
            continue;
        }

        Mesh* pMesh = m_meshes[i];
        pMesh->DrawMesh(m_pCommandList, backBufferIndex, true); //�[�x�݂̂�`��
    }
}

void Model::LateUpdate(UINT backBufferIndex)
{
    if (!m_bVisible) {
        return;
    }

    m_bShadowRendered = false;

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
    mcb.lightViewProjMatrix = m_pDirectionalLight->m_lightViewProj;
    mcb.normalMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, mcb.modelMatrix));
    mcb.cameraPos.x = m_pCamera->m_eyePos.m128_f32[0];
    mcb.cameraPos.y = m_pCamera->m_eyePos.m128_f32[0];
    mcb.cameraPos.z = m_pCamera->m_eyePos.m128_f32[0];
    mcb.cameraPos.w = 1.0f;
	const XMFLOAT3& lightPos = m_pDirectionalLight->m_position;
	mcb.lightPos.x = lightPos.x;
	mcb.lightPos.y = lightPos.y;
	mcb.lightPos.z = lightPos.z;
	mcb.lightPos.w = 1.0f;

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
            //�f�o�C�X�폜�̌��������O�ɏo��
            printf("Device removed reason: 0x%08X\n", reason);
        }
    }

    XMVECTOR objPos = XMLoadFloat3(&m_position);
    XMVECTOR camPos = m_pCamera->m_eyePos;
    m_depth = XMVectorGetZ(XMVector3Length(objPos - camPos)); //Z������[�x�Ƃ��Ď擾
}

Mesh::Mesh()
    : indexBufferView(D3D12_INDEX_BUFFER_VIEW())
    , vertexBufferView(D3D12_VERTEX_BUFFER_VIEW())
    , pMaterial(nullptr)
    , pModel(nullptr)
    , vertexCount(0)
    , indexCount(0)
    , shapeDataIndex(0)
    , bDraw(true)
{
}

Mesh::~Mesh()
{
}

void Mesh::DrawMesh(ID3D12GraphicsCommandList* pCommandList, UINT backBufferIndex, bool bShadowMode) const
{
    //�`��t���O�������Ă��Ȃ��ꍇ�͕`�悵�Ȃ�
    if (!bDraw) {
        return;
    }

    pCommandList->SetGraphicsRootConstantBufferView(0, pModel->GetConstantBuffer(backBufferIndex)->GetGPUVirtualAddress()); //���f���̈ʒu�֌W�𑗐M

    if (bShadowMode) {
        if (contentsBuffer) {
            pCommandList->SetGraphicsRootConstantBufferView(2, contentsBuffer->GetGPUVirtualAddress());      //���_���𑗐M
        }

        if (shapeDeltasBuffer) {
            pCommandList->SetGraphicsRootDescriptorTable(3, pMaterial->GetShapeData(shapeDataIndex));        //���_�V�F�[�_�[�̃V�F�C�v�L�[
        }

        if (shapeWeightsBuffer) {
            pCommandList->SetGraphicsRootShaderResourceView(4, shapeWeightsBuffer->GetGPUVirtualAddress());  //�V�F�C�v�L�[�̃E�F�C�g���𑗐M
        }
    }
    else {
        if (pModel->GetBoneBuffer()) {
            pCommandList->SetGraphicsRootConstantBufferView(5, pModel->GetBoneBuffer()->GetGPUVirtualAddress());   //�{�[���𑗐M
        }

        if (pModel->GetModelType() == ModelType_Character) {   //�{�[�������݂���V�F�[�_�[�̂�
            if (contentsBuffer) {
                pCommandList->SetGraphicsRootConstantBufferView(6, contentsBuffer->GetGPUVirtualAddress());      //���_���𑗐M
            }

            if (shapeWeightsBuffer) {
                pCommandList->SetGraphicsRootShaderResourceView(7, shapeWeightsBuffer->GetGPUVirtualAddress());  //�V�F�C�v�L�[�̃E�F�C�g���𑗐M
            }
        }
    }

    pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);           //���_���𑗐M
    pCommandList->IASetIndexBuffer(&indexBufferView);                    //�C���f�b�N�X���𑗐M
    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);  //3�p�|���S���̂�

    pCommandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);  //�`��
}
