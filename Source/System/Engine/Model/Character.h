#pragma once
#include <thread>
#include <future>
#include <mutex>

#include "Model.h"
#include "Animation\\Animation.h"
#include "..\\..\\Main\\Utility.h"
#include "..\\Engine.h"
#include "..\\Core\\BinaryFile\\BinaryCompression.h"

//�{�[�������݂��郂�f����ǂݍ���
//�V�F�C�v�L�[(Unity��BlendShape)�ɑΉ�
//Unity�œ����A�j���[�V�����ɑΉ� (�G�f�B�^�[�g����*.anim->*.hsc�ɕϊ�����K�v����)

class Engine;

class Character : public Model
{
public:

    //�q���[�}�m�C�h���f��
    struct HumanoidMesh {
        Mesh* pMesh;                                            //���b�V�����̂̃|�C���^
        std::unordered_map<std::string, UINT> shapeMapping;     //�V�F�C�v�L�[������C���f�b�N�X���擾
        std::vector<XMFLOAT3> shapeDeltas;                      //�e���_�̃V�F�C�v�L�[�K����̈ʒu���i�[ (���_�̐� * �V�F�C�v�L�[�̐������v�f�����m��)
        std::vector<float> shapeWeights;                        //���b�V�����Ƃ̃V�F�C�v�L�[�̃E�F�C�g�ꗗ
        UINT vertexCount;                                       //���b�V���̒��_��
        bool bChangeShapeValue;                                 //�E�F�C�g��ύX������true

        HumanoidMesh();
    };

    //�R���X�g���N�^
    //��{�I�ɃG���W���Ŏ��s
    //���� : std::string FBX�t�@�C���̃p�X, Camera* �J�����N���X�̃|�C���^, ID3D12Device* �f�o�C�X, ID3D12GraphicsCommandList* �R�}���h���X�g, Camera* �J����, 
    //          DirectionalLight* �f�B���N�V���i�����C�g, ID3D12Resource* �e���������܂��o�b�t�@
    Character(const std::string modelFile, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const Camera* pCamera, DirectionalLight* pDirectionalLight,
        ID3D12Resource* pShadowMapBuffer);
    ~Character();

    //�A�j���[�V������ǉ�
    //�߂�l : �ǉ����ꂽ�A�j���[�V�����̃C���f�b�N�X
    UINT AddAnimation(Animation animation);

    //�A�j���[�V�������X�V
    void Update();

    //�t���[���̍Ō�Ɏ��s (�G���W������Ă΂��)
    void LateUpdate(UINT backBufferIndex);

    //�{�[���̈ʒu���X�V
    void UpdateBonePosition(std::string boneName, XMFLOAT3& position);
    //�{�[���̉�]��ύX
    void UpdateBoneRotation(std::string boneName, XMFLOAT4& rotation);
    //�{�[���̃X�P�[����ύX
    void UpdateBoneScale(std::string boneName, XMFLOAT3& scale);

    //�V�F�C�v�L�[�̃E�F�C�g���X�V
    void SetShapeWeight(const std::string shapeName, float weight);

public: //�Q�b�^�[�֐�

    //�{�[�������擾
    inline Bone* GetBone(std::string boneName)
    {
        if (m_boneManager.m_boneMapping.find(boneName) == m_boneManager.m_boneMapping.end()) {
            return nullptr;
        }
        return &m_boneManager.m_bones[m_boneManager.m_boneMapping[boneName]];
    }
    //�{�[���̃I�t�Z�b�g���擾
    inline XMFLOAT3 GetBoneOffset(std::string boneName)
    {
        if (m_boneManager.m_boneMapping.find(boneName) == m_boneManager.m_boneMapping.end()) {
            return XMFLOAT3(0.0f, 0.0f, 0.0f);
        }

        XMMATRIX m = m_boneManager.m_bones[m_boneManager.m_boneMapping[boneName]].m_boneOffset;
        return XMFLOAT3(m.r[3].m128_f32[0], m.r[3].m128_f32[1], m.r[3].m128_f32[2]);
    }

    //�V�F�C�v�L�[�̃E�F�C�g���擾
    inline float GetShapeWeight(std::string shapeName)
    {
        //�����̃V�F�C�v�L�[�̓��b�V���Ԃŋ��L����Ă��邽�߁A�ŏ��Ɍ�������shapeName�̃V�F�C�v�L�[�̃E�F�C�g��Ԃ�
        for (HumanoidMesh& mesh : m_humanoidMeshes) {
            if (mesh.shapeMapping.find(shapeName) == mesh.shapeMapping.end())
                continue;

            return mesh.shapeWeights[mesh.shapeMapping[shapeName]];
        }

        return 0.0f;
    }
    inline float GetShapeWeight(std::string meshName, UINT shapeIndex)
    {
        for (HumanoidMesh& mesh : m_humanoidMeshes) {
            if (mesh.pMesh->meshName != meshName)
                continue;

            return mesh.shapeWeights[shapeIndex];
        }

        return 0.0f;
    }

    //���b�V���������ׂĎ擾
    inline std::vector<HumanoidMesh>& GetHumanMeshes()
    {
        return m_humanoidMeshes;
    }
    //���b�V�������烁�b�V�������擾
	inline HumanoidMesh* GetHumanMesh(std::string meshName)
	{
		for (HumanoidMesh& mesh : m_humanoidMeshes) {
			if (mesh.pMesh->meshName == meshName) {
				return &mesh;
			}
		}
		return nullptr;
	}

    //���݂̃A�j���[�V�����̒������擾
    inline float GetAnimationLength() const
    {
        if (m_animations.size() > m_nowAnimationIndex) {
            return 0.0f;
        }

        size_t frameCount = m_animations[m_nowAnimationIndex].m_frames.size();
        return m_animations[m_nowAnimationIndex].m_frames[frameCount - 1].time;
    }

public: //�p�u���b�N�ϐ�
    BoneManager m_boneManager;  //�{�[���Ɋւ������ێ�����\����   

    float m_animationSpeed;     //�A�j���[�V�������x�B1.0f���ʏ푬�x
    float m_nowAnimationTime;   //���݂̃A�j���[�V��������

private:
    //�V�F�[�_�[�ɓn�����_���
    struct Vertex {
        XMFLOAT3 position;      //���_�̈ʒu (T�|�[�Y)
        XMFLOAT4 boneWeights;   //�{�[���̉e���x
        UINT boneIDs[4];        //4�̃{�[������e�����󂯂�
        UINT vertexID;          //���_ID
        XMFLOAT3 normal;        //�@��
        XMFLOAT2 texCoords;     //UV
		XMFLOAT3 tangent;       //�ڐ�
		XMFLOAT3 bitangent;     //�]�@��
    };

    //�{�[�������݂���FBX�t�@�C�������[�h
    void LoadFBX(const std::string& fbxFile);
	//�{�[�������݂���HCS�t�@�C�������[�h
    void LoadHCS(const std::string& hcsFile);

    //�m�[�h��ǂݍ���
    void ProcessNode(const aiScene* pScene, const aiNode* pNode);
    //���b�V������.fbx�t�@�C������擾
    Mesh* ProcessMesh(aiMesh* mesh, HumanoidMesh& humanoidMesh);
    //���b�V������.hcs�t�@�C������擾
    void ProcessMesh(BinaryReader& br, HumanoidMesh& humanoidMesh);

    //�e�{�[���̃��[���h���W���Z�o
    void CalculateBoneTransforms(const aiNode* node, const XMMATRIX& parentTransform);

    //�{�[������.fbx����擾
    void LoadBones(aiMesh* mesh, std::vector<Vertex>& vertices);
    //�{�[������.hcs�t�@�C������擾
    void LoadBones(BinaryReader& br);
    //�{�[���̐e�q�֌W���擾(.fbx�̂�)
    void LoadBoneFamily(const aiNode* node);

    //�V�F�C�v�L�[�ƒ��_�̊֌W���擾(.fbx�̂�)
    void LoadShapeKey(const aiMesh* mesh, std::vector<Vertex>& vertices, HumanoidMesh& humanoidMesh);
    //HumanoidMesh�̓��e���擾 (.hcs�̂�)
    void LoadHumanoidMesh(BinaryReader& br);

    //�t�@�C������e�N�X�`�����쐬
    bool SetTexture(const Mesh* pMesh, const std::string nameOnly);

    //�V�F�C�v�L�[�̃E�F�C�g���X�V
    void UpdateShapeKeys();
    //�A�j���[�V�������X�V
    void UpdateAnimation();

    //�K�v�ȃo�b�t�@���쐬
    void CreateBuffer(Mesh* pMesh, std::vector<Vertex>& vertices, std::vector<UINT>& indices, HumanoidMesh& humanoidMesh);
    //�q���[�}�m�C�h�ɕK�v�ȃo�b�t�@���쐬
    void CreateHumanoidMeshBuffer(HumanoidMesh& humanoidMesh);
    //�V�F�C�v�L�[�̃f�[�^���e�N�X�`���Ƃ��č쐬 (�e�N�X�`���ɂ��邱�Ƃŏ������x�啝UP)
    void CreateShapeDeltasTexture(HumanoidMesh& humanoidMesh);

private:  //�v���C�x�[�g�ϐ�
    std::vector<HumanoidMesh> m_humanoidMeshes;             //�q���[�}�m�C�h�p�̃��b�V�����
    std::vector<Animation> m_animations;                    //�A�j���[�V�������

    int m_nowAnimationIndex;                                //���ݍĐ����̃A�j���[�V�����ԍ�
    bool bHCSFile;
};