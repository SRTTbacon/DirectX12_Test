#pragma once
#include <unordered_map>
#include "..\\Camera\\Camera.h"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include "Bone\\Bone.h"
#include "Material\MaterialManager.h"

using namespace DirectX;

//�{�[���͍ő�512��
constexpr int MAX_BONE_COUNT = 512;

//���f���t�@�C���̃w�b�_�[
constexpr const char* MODEL_HEADER = "HCSModel";
//���f���t�@�C�����L�����N�^�[�̏ꍇ��0
constexpr BYTE MODEL_CHARACTER = 0;

class Model;

enum ModelType
{
    ModelType_Unknown,      //���w��
    ModelType_Primitive,    //���ʂ̃��f��
    ModelType_Character,    //�{�[�������݂��郂�f��
};

//���b�V�����ƂɕK�v�ȏ��
struct Mesh {
    ComPtr<ID3D12Resource> vertexBuffer;            //���_�o�b�t�@(GPU�������ɒ��_����ۑ�)
    ComPtr<ID3D12Resource> indexBuffer;             //�C���f�b�N�X�o�b�t�@(GPU�������ɓ����Ă��钸�_�o�b�t�@�̊e���_�ɃC���f�b�N�X��t���ĕۑ�)
    ComPtr<ID3D12Resource> contentsBuffer;          //���_����V�F�C�v�L�[�̐��ȂǏ��������x�������M����p (�q���[�}�m�C�h���f���̂ݐݒ�)
    ComPtr<ID3D12Resource> shapeWeightsBuffer;      //�V�F�C�v�L�[�̃E�F�C�g���                             (�q���[�}�m�C�h���f���̂ݐݒ�)
    ComPtr<ID3D12Resource> shapeDeltasBuffer;       //�e���_�ɑ΂���V�F�C�v�L�[�̈ʒu���                   (�q���[�}�m�C�h���f���̂ݐݒ�)
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;      //���_�o�b�t�@�̃f�[�^���e�ƃT�C�Y��ێ�
    D3D12_INDEX_BUFFER_VIEW indexBufferView;        //�C���f�b�N�X�o�b�t�@�̃f�[�^���e�ƃT�C�Y��ێ�
    Material* pMaterial;                            //�}�e���A��
    Model* pModel;                                  //�e�ł��郂�f���N���X
    std::string meshName;                           //���b�V����
    UINT vertexCount;                               //���_��
    UINT indexCount;                                //�C���f�b�N�X�� (GPU���ŁA���̐��Ԃ�`�悳����)
    UINT shapeDataIndex;                            //�f�B�X�N���v�^�q�[�v��̃V�F�C�v�L�[�̃f�[�^�����݂���ꏊ
	bool bDraw;                                     //�`�悷�邩�ǂ���

    Mesh();
    ~Mesh();

    void DrawMesh(ID3D12GraphicsCommandList* pCommandList, UINT backBufferIndex, bool bShadowMode = false) const;
};

class Model
{
public:
    //���f����������
    Model(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const Camera* pCamera, const DirectionalLight* pDirectionalLight, MaterialManager* pMaterialManager);
    ~Model();

    void LoadModel(const std::string fbxFile);

    //�X�V (�G���W��������s����邽�߁A���[�U�[�����s����K�v�͂Ȃ�)
    virtual void LateUpdate(UINT backBufferIndex);

    //�V���h�E�}�b�v�ɕ`�� (�G���W��������s����邽�߁A���[�U�[�����s����K�v�͂Ȃ�)
    void RenderShadowMap(UINT backBufferIndex);

    XMFLOAT3 m_position;    //���f���S�̂̈ʒu
    XMFLOAT3 m_rotation;    //���f���S�̂̉�] (�f�O���[�p)
    XMFLOAT3 m_scale;       //���f���S�̂̃X�P�[��

    bool m_bVisible;        //�`�悷�邩�ǂ���

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

    //�[�x
    inline float GetZBuffer() const { return m_depth; }

    //���ɉe�̃����_�����O���I����Ă��邩
    inline bool GetIsShadowRendered() const { return m_bShadowRendered; }

    //���f���̃t�@�C����
    inline std::string GetModelFilePath() const { return m_modelFile; }

protected:
    //�V�F�[�_�[�ɓn�����_��� (�v���~�e�B�u�p)
    struct VertexPrimitive {
        XMFLOAT3 position;
        XMFLOAT4 boneWeights;   //�e�p�̃V�F�[�_�[�ƍ��킹��K�v�����邽�߃_�~�[
        UINT boneIDs[4];        //�e�p�̃V�F�[�_�[�ƍ��킹��K�v�����邽�߃_�~�[
        UINT vertexID;          //���_ID
        XMFLOAT3 normal;
        XMFLOAT2 texCoords;
		XMFLOAT3 tangent;
		XMFLOAT3 bitangent;
    };

    //�V�F�[�_�[�ɓn���r���[���
    struct ModelConstantBuffer {
        XMMATRIX modelMatrix;
        XMMATRIX viewMatrix;
        XMMATRIX projectionMatrix;
        XMMATRIX lightViewProjMatrix;
        XMMATRIX normalMatrix;
		XMFLOAT4 cameraPos;
        XMFLOAT4 lightPos;
    };

    //�V�F�[�_�[�ɓn�����_���̏��
    struct Contents {
        UINT vertexCount;
        UINT shapeCount;
    };

    ID3D12Device* m_pDevice;                                            //�G���W���̃f�o�C�X
    ID3D12GraphicsCommandList* m_pCommandList;                          //�G���W���̃R�}���h���X�g
    ComPtr<ID3D12Resource> m_modelConstantBuffer[FRAME_BUFFER_COUNT];   //�R���X�^���g�o�b�t�@�B��ʂ̂������h�~���邽�߃g���v���o�b�t�@�����O (2�ł��\���Ȃ̂���?)
    ComPtr<ID3D12Resource> m_boneMatricesBuffer;                        //�{�[�������V�F�[�_�[�ɑ��M����p
    ComPtr<ID3D12Resource> m_shadowBoneMatricesBuffer;                  //�e�p�V�F�[�_�[�Ƀ{�[�����𑗐M����p(�{�[�����Ȃ��ꍇ��0�ŏ�����)

    std::vector<Mesh*> m_meshes;    //���b�V���̔z��

    const Camera* m_pCamera;                        //�J�������
    const DirectionalLight* m_pDirectionalLight;    //�f�B���N�V���i�����C�g���
    MaterialManager* m_pMaterialManager;            //�}�e���A���쐬&�擾

    XMMATRIX m_modelMatrix;                 //�ʒu�A��]�A�X�P�[����Matrix�ŕێ�

    ModelType m_modelType;
    std::string m_modelFile;

    float m_depth;          //Z�o�b�t�@

    void CreateConstantBuffer();

    template <typename VertexType>
    void CreateBuffer(Mesh* pMesh, std::vector<VertexType>& vertices, std::vector<UINT>& indices, UINT vertexStructSize);

private:
    bool m_bShadowRendered;     //�e�̃����_�����O���I����Ă��邩�ǂ���

    void ProcessNode(const aiScene* scene, aiNode* node);   //�m�[�h��ǂݍ���
    Mesh* ProcessMesh(const aiScene* scene, aiMesh* mesh);  //���b�V������ǂݍ���
};

//�K�v�ȃo�b�t�@���쐬 (�L�����N�^�[�ƕ��ʂ̃��f���������ɑΉ�)
template <typename VertexType>
void Model::CreateBuffer(Mesh* pMesh, std::vector<VertexType>& vertices, std::vector<UINT>& indices, UINT vertexStructSize)
{
    const UINT bufferSize = static_cast<UINT>(vertexStructSize * vertices.size());

    pMesh->vertexCount = static_cast<UINT>(vertices.size());

    //�q�[�v�ݒ�
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    //���_�o�b�t�@�̃��\�[�X
    CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    //�f�o�C�X�ō쐬
    HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pMesh->vertexBuffer));

    if (FAILED(hr)) {
        printf("���_�o�b�t�@�̐����Ɏ��s���܂����B%1xl\n", hr);
    }

    //���_�f�[�^��GPU�ɑ��M
    void* vertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    pMesh->vertexBuffer->Map(0, &readRange, &vertexDataBegin);
    memcpy(vertexDataBegin, vertices.data(), bufferSize);
    pMesh->vertexBuffer->Unmap(0, nullptr);

    pMesh->vertexBufferView.BufferLocation = pMesh->vertexBuffer->GetGPUVirtualAddress();
    pMesh->vertexBufferView.StrideInBytes = vertexStructSize;
    pMesh->vertexBufferView.SizeInBytes = bufferSize;

    //�C���f�b�N�X�o�b�t�@�̍쐬
    const UINT indexBufferSize = static_cast<UINT>(sizeof(UINT) * indices.size());

    //�C���f�b�N�X�o�b�t�@�̃��\�[�X
    CD3DX12_RESOURCE_DESC indexBuffer = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

    hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &indexBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pMesh->indexBuffer));

    if (FAILED(hr)) {
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

    //���_����ێ�����p�̃��\�[�X���쐬
    CD3DX12_RESOURCE_DESC contentsBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Contents));
    hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &contentsBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pMesh->contentsBuffer));
    if (FAILED(hr)) {
        printf("�R���e���c�o�b�t�@�̐����Ɏ��s���܂����B\n");
    }

    //���_���ƃV�F�C�v�L�[����ێ�
    Contents contents{};
    contents.vertexCount = 1;
    contents.shapeCount = 0;
    void* pContentsBuffer;
    pMesh->contentsBuffer->Map(0, nullptr, &pContentsBuffer);
    if (pContentsBuffer)
        memcpy(pContentsBuffer, &contents, sizeof(Contents));
    pMesh->contentsBuffer->Unmap(0, nullptr);
}