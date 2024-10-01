#pragma once
#include <assimp/scene.h>
#include <string>
#include <unordered_map>
#include "..\\Core\\RootSignature\\RootSignature.h"
#include "..\\Core\\PipelineState\\PipelineState.h"
#include "..\\Core\\DescriptorHeap\\DescriptorHeap.h"
#include "..\\Core\\Texture2D\\Texture2D.h"
#include "..\\Camera\\Camera.h"

using namespace DirectX;

//�{�[���͍ő�512��
constexpr int MAX_BONE_COUNT = 512;

//�V�F�[�_�[�ɓn�����_���
struct Vertex {
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT2 TexCoords;
    XMFLOAT4 Color; // ���_�F
    XMFLOAT4 BoneWeights;
    UINT BoneIDs[4];
};

//�V�F�[�_�[�ɓn���r���[���
struct ModelConstantBuffer {
    XMMATRIX modelMatrix;
    XMMATRIX viewMatrix;
    XMMATRIX projectionMatrix;
};

class Model
{
public:
    Model(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::string& fbxFile, const Camera* pCamera);

    //�X�V
    void Update();

    //�`��
    void Draw(ID3D12GraphicsCommandList* commandList);

    //�{�[���̈ʒu���X�V
    void UpdateBonePosition(std::string boneName, XMFLOAT3& position);
    //�{�[���̉�]��ύX
    void UpdateBoneRotation(std::string boneName, XMFLOAT3& rotation);
    //�{�[���̃X�P�[����ύX
    void UpdateBoneScale(std::string boneName, XMFLOAT3& scale);

    XMFLOAT3 m_position;    //���f���S�̂̈ʒu
    XMFLOAT3 m_rotation;    //���f���S�̂̉�] (�f�O���[�p)
    XMFLOAT3 m_scale;       //���f���S�̂̃X�P�[��

private:
    struct BoneNode {
        std::string boneName;           //�{�[����
        std::vector<UINT> childBones;   //�q�{�[��
        XMMATRIX boneOffset;            //�{�[���̌��_�Ƃ̍�
        UINT parentBoneIndex;           //�e�{�[���̃C���f�b�N�X
        XMFLOAT3 m_position;            //�{�[���̈ʒu
        XMFLOAT3 m_rotation;            //�{�[���̉�] (�f�O���[�p)
        XMFLOAT3 m_scale;               //�{�[���̃X�P�[��
    };

    struct Mesh {
        ComPtr<ID3D12Resource> vertexBuffer;            //���_�o�b�t�@(GPU�������ɒ��_����ۑ�)
        ComPtr<ID3D12Resource> indexBuffer;             //�C���f�b�N�X�o�b�t�@(GPU�������ɓ����Ă��钸�_�o�b�t�@�̊e���_�ɃC���f�b�N�X��t���ĕۑ�)
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView;      //���_�o�b�t�@�̃f�[�^���e�ƃT�C�Y��ێ�
        D3D12_INDEX_BUFFER_VIEW indexBufferView;        //�C���f�b�N�X�o�b�t�@�̃f�[�^���e�ƃT�C�Y��ێ�
        UINT indexCount;                                //�C���f�b�N�X�� (GPU���ŁA���̐��Ԃ�`�悳����)
        BYTE materialIndex;                             //�}�e���A���������Ă���C���f�b�N�X (�����e�N�X�`���͎g���܂킷)
    };

    ID3D12Device* device;                                           //�G���W���̃f�o�C�X
    RootSignature* m_pRootSignature;                                //���[�g�V�O�l�`��
    PipelineState* m_pPipelineState;                                //�p�C�v���C���X�e�[�g
    DescriptorHeap* descriptorHeap;                                 //�}�e���A��
    ComPtr<ID3D12Resource> modelConstantBuffer[FRAME_BUFFER_COUNT]; //��ʂ̂������h�~���邽�߃g���v���o�b�t�@�����O
    ComPtr<ID3D12Resource> m_boneMatricesBuffer;

    const Camera* m_pCamera;                        //�J�������

    std::vector<Mesh> meshes;                           //���b�V���̔z�� (�L�����N�^�[�ȂǁAFBX���ɕ����̃��b�V�������݂�����̂ɑΉ�)
    std::vector<XMMATRIX> boneInfos;                    //�V�F�[�_�[�ɑ��M����{�[���̃}�g���b�N�X
    std::vector<XMMATRIX> finalBoneInfos;
    std::vector<BoneNode> boneWorlds;                   //�{�[�����
    std::unordered_map<std::string, UINT> boneMapping;  //�{�[��������C���f�b�N�X���擾

    XMMATRIX m_modelMatrix;         //�ʒu�A��]�A�X�P�[����Matrix�ŕێ�

    void LoadFBX(const std::string& fbxFile);               //FBX�����[�h
    void ProcessNode(const aiScene* scene, aiNode* node);
    Mesh ProcessMesh(const aiScene* scene, aiMesh* mesh);
    void LoadBones(const aiScene* scene, Mesh& meshStruct, aiMesh* mesh, std::vector<Vertex>& vertices);    //�{�[�������擾
    void LoadBoneFamily(const aiNode* node);        //�{�[���̐e�q�֌W���擾
    void UpdateBoneTransform(UINT boneIndex, XMMATRIX& parentMatrix);   //�V�F�[�_�[�ɓn���{�[���̍��W���v�Z
    void UpdateBoneTransform();                                         //�V�F�[�_�[�ɓn���{�[���̍��W���v�Z
};