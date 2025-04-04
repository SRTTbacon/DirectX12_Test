#pragma once

#include <unordered_map>
#include <fstream>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "..\\Core\\ResourceCopy\\ResourceCopy.h"
#include "..\\Camera\\Camera.h"
#include "Animation\\Animation.h"
#include "Bone\\Bone.h"
#include "Material\\MaterialManager.h"
#include "Transform\\Transform.h"
#include "Mesh.h"

using namespace DirectX;

//���f���t�@�C���̃w�b�_�[
constexpr const char* MODEL_HEADER = "HCSModel";
//���f���t�@�C�����L�����N�^�[�̏ꍇ��0
constexpr BYTE MODEL_CHARACTER = 0;
constexpr BYTE MODEL_DEFAULT = 1;

class Model;

enum ModelType
{
    ModelType_Unknown,      //���w��
    ModelType_Primitive,    //���ʂ̃��f��
    ModelType_Character,    //�{�[�������݂��郂�f��
};

//���b�V�����ƂɕK�v�ȏ��
struct Mesh : public Transform {

    //���b�V���̃}�g���b�N�X���
    struct MeshCostantBuffer
    {
        XMMATRIX meshMatrix;    //���b�V���̈ʒu�A�p�x�A�X�P�[��
        XMMATRIX normalMatrix;  //���b�V���ƃ��f���S�̂̃}�g���b�N�X���|��������
        float time;             //���ԏ��i�V�F�[�_�[�œ��I�ɍX�V�j
    };

    std::shared_ptr<MeshData> meshData;             //���L���b�V���f�[�^

    ComPtr<ID3D12Resource> shapeWeightsBuffer;      //�V�F�C�v�L�[�̃E�F�C�g���              (�q���[�}�m�C�h���f���̂ݐݒ�)
    void* pShapeWeightsMapped;                      //�V�F�C�v�L�[�̃E�F�C�g����Map         (�q���[�}�m�C�h���f���̂ݐݒ�)

    Material* pMaterial;                            //�}�e���A��
    Model* pModel;                                  //�e�ł��郂�f���N���X

    std::string meshName;                           //���b�V����

    bool bDraw;                                     //�`�悷�邩�ǂ���
    bool bDrawShadow;                               //�e��`�悷�邩�ǂ���

    Mesh();
    ~Mesh();

    //���b�V����`��
    //ModelManager������s�����
    void DrawMesh(ID3D12GraphicsCommandList* pCommandList, UINT backBufferIndex, XMMATRIX& modelMatrix, bool bShadowMode = false);
};

class Model : public Transform
{
    friend class ModelManager;

public:
    //���f����������
    Model(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const Camera* pCamera, const DirectionalLight* pDirectionalLight, MaterialManager* pMaterialManager, float* pFrameTime);
    virtual ~Model();

    //FBX�t�@�C�����烂�f�����쐬
    void LoadModel(const std::string modelFile);

    virtual void Update();

public: //�Q�b�^�[�֐�
    //���b�V����
    inline UINT GetMeshCount() const { return static_cast<UINT>(m_meshes.size()); }

    //���b�V�����擾
    inline Mesh* GetMesh(UINT index) { return m_meshes[index]; }

    //���f����񂪓����Ă���o�b�t�@���擾
    inline ID3D12Resource* GetConstantBuffer(UINT backBufferIndex) const { return m_modelConstantBuffer[backBufferIndex].Get(); }

    //�{�[���o�b�t�@���擾
    inline ID3D12Resource* GetBoneBuffer() const { return m_boneMatricesBuffer.Get(); }

    //���f���^�C�v���擾
    inline ModelType GetModelType() const { return m_modelType; }

    //�[�x���擾
    inline float GetZBuffer() const { return m_depth; }

    //���ɉe�̃����_�����O���I����Ă��邩
    inline bool GetIsShadowRendered() const { return m_bShadowRendered; }

    //���f���̃t�@�C����
    inline std::string GetModelFilePath() const { return m_modelFile; }

    //���f�������݂��Ă��鎞�Ԃ��擾
    inline float GetModelTime() const { return m_modelTime; }

public:
    float m_animationSpeed;     //�A�j���[�V�������x�B1.0f���ʏ푬�x
    float m_nowAnimationTime;   //���݂̃A�j���[�V��������

    bool m_bDrawShadow;

protected:
    //�V�F�[�_�[�ɓn�����_��� (�v���~�e�B�u�p)
    struct VertexPrimitive {
        XMFLOAT3 position;
        XMFLOAT4 boneWeights;   //�e�p�̃V�F�[�_�[�ƍ��킹��K�v�����邽�߃_�~�[
        UINT boneIDs[4];        //�e�p�̃V�F�[�_�[�ƍ��킹��K�v�����邽�߃_�~�[
        UINT vertexID;          //���_ID (Index)
        XMFLOAT3 normal;
        XMFLOAT2 texCoords;
		XMFLOAT3 tangent;
		XMFLOAT3 bitangent;
    };

    //�V�F�[�_�[�ɓn�����_���̏��
    struct Contents {
        UINT vertexCount;
        UINT shapeCount;
    };

    //���b�V�������L���邽�߂̍\����
    struct MeshDataList
    {
        struct MeshDataMain
        {
            std::string meshName;
            DirectX::XMFLOAT3 position;
            DirectX::XMFLOAT3 rotation;
        };

        int refCount;
        std::vector<std::shared_ptr<MeshData>> meshDataList;
        std::vector<MeshDataMain> meshMain;
        std::vector<Animation> animations;

        MeshDataList()
            : refCount(1)
        {
        }

        void Release()
        {
            refCount--;
            if (refCount <= 0) {
                meshDataList.clear();
            }
        }
    };
    static std::unordered_map<std::string, MeshDataList> s_sharedMeshes;    //�e���f���̃��b�V����ێ�

    ID3D12Device* m_pDevice;                                            //�G���W���̃f�o�C�X
    ID3D12GraphicsCommandList* m_pCommandList;                          //�G���W���̃R�}���h���X�g
    ComPtr<ID3D12Resource> m_modelConstantBuffer[FRAME_BUFFER_COUNT];   //�R���X�^���g�o�b�t�@�B��ʂ̂������h�~���邽�߃g���v���o�b�t�@�����O (2�ł��\���Ȃ̂���?)
    void* m_pMappedConstantBuffer[FRAME_BUFFER_COUNT];
    ComPtr<ID3D12Resource> m_boneMatricesBuffer;                        //�{�[�������V�F�[�_�[�ɑ��M����p
    void* m_pBoneMatricesMap;                                           //�{�[�����̃|�C���^

    std::vector<Mesh*> m_meshes;    //���b�V���̔z��

    const Camera* m_pCamera;                        //�J�������
    const DirectionalLight* m_pDirectionalLight;    //�f�B���N�V���i�����C�g���
    MaterialManager* m_pMaterialManager;            //�}�e���A���쐬&�擾

    ModelType m_modelType;
    std::string m_modelFile;

    const float* m_pFrameTime;

    int m_nowAnimationIndex;        //���ݍĐ����̃A�j���[�V�����ԍ�

    float m_depth;                  //Z�o�b�t�@

protected:
    void CreateConstantBuffer();

    template <typename VertexType>
    void CreateBuffer(Mesh* pMesh, std::vector<VertexType>& vertices, std::vector<UINT>& indices, UINT vertexStructSize, UINT meshIndex);

    //�X�V (�G���W��������s����邽�߁A���[�U�[�����s����K�v�͂Ȃ�)
    virtual void LateUpdate(UINT backBufferIndex);

private:
    //�V�F�[�_�[�ɓn���r���[���
    struct ModelConstantBuffer {
        XMMATRIX modelMatrix;
        XMMATRIX viewMatrix;
        XMMATRIX projectionMatrix;
        XMMATRIX lightViewProjMatrix;
        XMFLOAT4 cameraPos;
        XMFLOAT4 lightPos;
    };

    std::vector<Animation> m_animations;

    ModelConstantBuffer m_modelConstantStruct;  //���_�V�F�[�_�[�ɑ��M���郁�b�V���ϐ�

    float m_modelTime;          //���f�������݂��Ă��鎞�� (�b)
    bool m_bShadowRendered;     //�e�̃����_�����O���I����Ă��邩�ǂ���

private:
    void ProcessNode(const aiScene* scene, aiNode* node);   //�m�[�h��ǂݍ���
    Mesh* ProcessMesh(const aiScene* scene, aiMesh* mesh, UINT meshIndex);  //���b�V������ǂݍ���
    Mesh* ProcessMesh(BinaryReader& br, UINT meshIndex);
    void ProcessAnimation(const aiScene* pScene);
    void ProcessAnimation(BinaryReader& br);

    void UpdateAnimation();

    bool SetSharedMeshes();

    XMMATRIX GetMeshDefaultMatrix(aiNode* pNode);

    //�V���h�E�}�b�v�ɕ`�� (�G���W��������s����邽�߁A���[�U�[�����s����K�v�͂Ȃ�)
    void RenderShadowMap(UINT backBufferIndex);
};

//�K�v�ȃo�b�t�@���쐬 (�L�����N�^�[�ƕ��ʂ̃��f���������ɑΉ�)
template <typename VertexType>
void Model::CreateBuffer(Mesh* pMesh, std::vector<VertexType>& vertices, std::vector<UINT>& indices, UINT vertexStructSize, UINT meshIndex)
{
    if (s_sharedMeshes.find(m_modelFile) != s_sharedMeshes.end()) {
        if (meshIndex == 0) {
            s_sharedMeshes[m_modelFile].refCount++;
        }
        if (s_sharedMeshes[m_modelFile].meshDataList.size() > meshIndex) {
            size_t size = s_sharedMeshes[m_modelFile].meshDataList.size();
            pMesh->meshData = s_sharedMeshes[m_modelFile].meshDataList[meshIndex];
            pMesh->meshName = s_sharedMeshes[m_modelFile].meshMain[meshIndex].meshName;
            pMesh->m_position = s_sharedMeshes[m_modelFile].meshMain[meshIndex].position;
            pMesh->m_rotation = s_sharedMeshes[m_modelFile].meshMain[meshIndex].rotation;
            return;
        }
    }
    else {
        s_sharedMeshes[m_modelFile] = MeshDataList();
        s_sharedMeshes[m_modelFile].meshDataList = std::vector<std::shared_ptr<MeshData>>();
        s_sharedMeshes[m_modelFile].meshMain= std::vector<MeshDataList::MeshDataMain>();
        if (meshIndex == 0) {
            s_sharedMeshes[m_modelFile].refCount = 1;
        }
    }

    const UINT bufferSize = static_cast<UINT>(vertexStructSize * vertices.size());

    std::shared_ptr<MeshData> p1(new MeshData());
    s_sharedMeshes[m_modelFile].meshDataList.push_back(p1);
    MeshDataList::MeshDataMain dataMain{};
    s_sharedMeshes[m_modelFile].meshMain.push_back(dataMain);

    pMesh->meshData = p1;
    pMesh->meshData->vertexCount = static_cast<UINT>(vertices.size());

    //�q�[�v�ݒ�
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    //���_�o�b�t�@�̃��\�[�X
    CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    //�f�o�C�X�ō쐬
    HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pMesh->meshData->vertexBuffer));

    if (FAILED(hr)) {
        printf("���_�o�b�t�@�̐����Ɏ��s���܂����B%1xl\n", hr);
    }

    ID3D12Resource* pUploadBuffer = g_resourceCopy->CreateUploadBuffer(bufferSize);

    //���\�[�X�̃R�s�[�͔񓯊��ōs��
    std::thread([=] {
        //���_�f�[�^��GPU�ɑ��M
        void* vertexDataBegin;
        pUploadBuffer->Map(0, nullptr, &vertexDataBegin);
        memcpy(vertexDataBegin, vertices.data(), bufferSize);
        pUploadBuffer->Unmap(0, nullptr);

        g_resourceCopy->BeginCopyResource();
        g_resourceCopy->GetCommandList()->CopyBufferRegion(pMesh->meshData->vertexBuffer.Get(), 0, pUploadBuffer, 0, bufferSize);
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pMesh->meshData->vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        g_resourceCopy->GetCommandList()->ResourceBarrier(1, &barrier);
        g_resourceCopy->EndCopyResource();
        pUploadBuffer->Release();
        }).detach();

    pMesh->meshData->vertexBufferView.BufferLocation = pMesh->meshData->vertexBuffer->GetGPUVirtualAddress();
    pMesh->meshData->vertexBufferView.StrideInBytes = vertexStructSize;
    pMesh->meshData->vertexBufferView.SizeInBytes = bufferSize;

    //�C���f�b�N�X�o�b�t�@�̍쐬
    const UINT indexBufferSize = static_cast<UINT>(sizeof(UINT) * indices.size());

    //�C���f�b�N�X�o�b�t�@�̃��\�[�X
    CD3DX12_RESOURCE_DESC indexBuffer = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

    hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pMesh->meshData->indexBuffer));

    if (FAILED(hr)) {
        printf("�C���f�b�N�X�o�b�t�@�̐����Ɏ��s���܂����B\n");
    }

    pUploadBuffer = g_resourceCopy->CreateUploadBuffer(indexBufferSize);

    //���\�[�X�̃R�s�[�͔񓯊��ōs��
    std::thread([=] {
        //�C���f�b�N�X�f�[�^��GPU�ɑ��M
        void* indexDataBegin;
        pUploadBuffer->Map(0, nullptr, &indexDataBegin);
        memcpy(indexDataBegin, indices.data(), indexBufferSize);
        pUploadBuffer->Unmap(0, nullptr);

        g_resourceCopy->BeginCopyResource();
        g_resourceCopy->GetCommandList()->CopyBufferRegion(pMesh->meshData->indexBuffer.Get(), 0, pUploadBuffer, 0, indexBufferSize);
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pMesh->meshData->indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        g_resourceCopy->GetCommandList()->ResourceBarrier(1, &barrier);
        g_resourceCopy->EndCopyResource();
        pUploadBuffer->Release();
        }).detach();

    pMesh->meshData->indexBufferView.BufferLocation = pMesh->meshData->indexBuffer->GetGPUVirtualAddress();
    pMesh->meshData->indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    pMesh->meshData->indexBufferView.SizeInBytes = indexBufferSize;

    pMesh->meshData->indexCount = static_cast<UINT>(indices.size());

    //���_����ێ�����p�̃��\�[�X���쐬
    CD3DX12_RESOURCE_DESC contentsBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Contents));
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &contentsBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pMesh->meshData->contentsBuffer));
    if (FAILED(hr)) {
        printf("�R���e���c�o�b�t�@�̐����Ɏ��s���܂����B\n");
    }

    //���_���ƃV�F�C�v�L�[����ێ�
    Contents contents{};
    contents.vertexCount = 1;
    contents.shapeCount = 0;
    void* pContentsBuffer;
    pMesh->meshData->contentsBuffer->Map(0, nullptr, &pContentsBuffer);
    if (pContentsBuffer)
        memcpy(pContentsBuffer, &contents, sizeof(Contents));
    pMesh->meshData->contentsBuffer->Unmap(0, nullptr);
}
