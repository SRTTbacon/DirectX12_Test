#pragma once
#include <unordered_map>
#include "..\\Core\\RootSignature\\RootSignature.h"
#include "..\\Core\\PipelineState\\PipelineState.h"
#include "..\\Core\\DescriptorHeap\\DescriptorHeap2.h"
#include "..\\Core\\Texture2D\\Texture2D.h"
#include "..\\Camera\\Camera.h"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include "Bone\\Bone.h"
#include "..\\Lights\\DirectionalLight.h"

using namespace DirectX;

//�{�[���͍ő�512��
constexpr int MAX_BONE_COUNT = 512;

enum PrimitiveModel
{
    Primitive_None,
    Primitive_Sphere
};

class Model
{
public:
    //���f����������
    Model(const Camera* pCamera, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, DirectionalLight* pDirectionalLight, UINT* pBackBufferIndex);

    void LoadModel(const PrimitiveModel primitiveModel);
    void LoadModel(const std::string fbxFile);

    //�X�V (�G���W��������s����邽�߁A���[�U�[�����s����K�v�͂Ȃ�)
    virtual void Update();

    //�V���h�E�}�b�v�ɕ`�� (�G���W��������s����邽�߁A���[�U�[�����s����K�v�͂Ȃ�)
    void RenderShadowMap();
    //���ۂɕ`�� (�G���W��������s����邽�߁A���[�U�[�����s����K�v�͂Ȃ�)
    void RenderSceneWithShadow();

    XMFLOAT3 m_position;    //���f���S�̂̈ʒu
    XMFLOAT3 m_rotation;    //���f���S�̂̉�] (�f�O���[�p)
    XMFLOAT3 m_scale;       //���f���S�̂̃X�P�[��

public: //�Q�b�^�[�֐�
    //�[�x
    inline float GetZBuffer() const
    {
        return m_depth;
    }

    //���������ǂ���
    inline bool GetIsTransparent() const
    {
        return m_bTransparent;
    }

protected:
    //�V�F�[�_�[�ɓn�����_��� (�v���~�e�B�u�p)
    struct VertexPrimitive {
        XMFLOAT3 Position;
        XMFLOAT3 Normal;
        XMFLOAT2 TexCoords;
        XMFLOAT4 Color; // ���_�F
    };

    //�V�F�[�_�[�ɓn���r���[���
    struct ModelConstantBuffer {
        XMMATRIX modelMatrix;
        XMMATRIX viewMatrix;
        XMMATRIX projectionMatrix;
        XMMATRIX lightViewProjMatrix;
        XMMATRIX normalMatrix;
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
        UINT indexCount;                                //�C���f�b�N�X�� (GPU���ŁA���̐��Ԃ�`�悳����)
        //char materialIndex;                             //�}�e���A���������Ă���C���f�b�N�X (�����e�N�X�`���͎g���܂킷)

        Mesh();

        void Draw(ID3D12GraphicsCommandList* pCommandList) const;
    };

    ID3D12Device* m_pDevice;                                            //�G���W���̃f�o�C�X
    ID3D12GraphicsCommandList* m_pCommandList;                          //�G���W���̃R�}���h���X�g
    RootSignature* m_pRootSignature;                                    //���[�g�V�O�l�`��
    PipelineState* m_pPipelineState;                                    //�p�C�v���C���X�e�[�g
    DescriptorHeap* m_pDescriptorHeap;                                  //�}�e���A��
    ComPtr<ID3D12Resource> m_modelConstantBuffer[FRAME_BUFFER_COUNT];   //�R���X�^���g�o�b�t�@�B��ʂ̂������h�~���邽�߃g���v���o�b�t�@�����O (2�ł��\���Ȃ̂���?)
    ComPtr<ID3D12Resource> m_boneMatricesBuffer;                        //�{�[�������V�F�[�_�[�ɑ��M����p
    ComPtr<ID3D12Resource> m_lightConstantBuffer;                       //�f�B���N�V���i�����C�g�̃o�b�t�@

    UINT* m_pBackBufferIndex;        //�G���W���̃o�b�N�o�b�t�@�̃C���f�b�N�X

    const Camera* m_pCamera;        //�J�������
    const DirectionalLight* m_pDirectionalLight;    //�f�B���N�V���i�����C�g

    std::vector<Mesh*> m_meshes;    //���b�V���̔z�� (�L�����N�^�[�ȂǁAFBX���ɕ����̃��b�V�������݂�����̂ɑΉ�)

    XMMATRIX m_modelMatrix;         //�ʒu�A��]�A�X�P�[����Matrix�ŕێ�

private:
    void LoadSphere(float radius, UINT sliceCount, UINT stackCount, const XMFLOAT4 color);
    void CreateBuffer(Mesh* pMesh, std::vector<VertexPrimitive>& vertices, std::vector<UINT>& indices);

    void ProcessNode(const aiScene* scene, aiNode* node);   //�m�[�h��ǂݍ���
    Mesh* ProcessMesh(const aiScene* scene, aiMesh* mesh);   //���b�V������ǂݍ���

    void CreateDirectionalLightBuffer();

    float m_depth;          //Z�o�b�t�@
    bool m_bTransparent;    //�������̃I�u�W�F�N�g���ǂ���
};