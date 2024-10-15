#pragma once
#include "Model.h"
#include "..\\..\\Main\\Utility.h"

class Character : public Model
{
public:
	Character(const std::string fbxFile, const Camera* pCamera);

    void LoadAnimation(std::string animFile);

    void Update();

    //�{�[���̈ʒu���X�V
    void UpdateBonePosition(std::string boneName, XMFLOAT3& position);
    //�{�[���̉�]��ύX
    void UpdateBoneRotation(std::string boneName, XMFLOAT4& rotation);
    //�{�[���̃X�P�[����ύX
    void UpdateBoneScale(std::string boneName, XMFLOAT3& scale);

    //�V�F�C�v�L�[�̃E�F�C�g���X�V
    void SetShapeWeight(const std::string shapeName, float weight);

public: //�Q�b�^�[�֐� (�p�ɂɌĂяo�����̂�inline)

    //���ׂẴ{�[�������擾
    std::vector<std::string> GetBoneNames();

    //�{�[�������擾
    inline Bone* GetBone(std::string boneName)
    {
        if (m_boneMapping.find(boneName) == m_boneMapping.end()) {
            return nullptr;
        }
        return &m_bones[m_boneMapping[boneName]];
    }
    //�{�[���̃I�t�Z�b�g���擾
    inline XMFLOAT3 GetBoneOffset(std::string boneName)
    {
        if (m_boneMapping.find(boneName) == m_boneMapping.end()) {
            return XMFLOAT3(0.0f, 0.0f, 0.0f);
        }

        XMMATRIX m = m_bones[m_boneMapping[boneName]].GetBoneOffset();
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

public: //�p�u���b�N�ϐ�
    float m_animationSpeed;

private:
    //�V�F�[�_�[�ɓn�����_���
    struct Vertex {
        XMFLOAT3 position;      //���_�̈ʒu (T�|�[�Y)
        XMFLOAT3 normal;        //�@��
        XMFLOAT2 texCoords;     //UV
        XMFLOAT4 color;         //�F
        XMFLOAT4 boneWeights;   //�{�[���̉e���x
        UINT boneIDs[4];        //4�̃{�[������e�����󂯂�
        UINT vertexID;          //���_ID
    };
    //�V�F�[�_�[�ɓn�����_���̏�� (�\���̂œn���K�v?)
    struct Contents {
        UINT vertexCount;
        UINT shapeCount;
    };
    //�q���[�}�m�C�h���f��
    struct HumanoidMesh {
        Mesh* pMesh;                                            //���b�V�����̂̃|�C���^
        std::unordered_map<std::string, UINT> shapeMapping;     //�V�F�C�v�L�[������C���f�b�N�X���擾
        std::vector<XMFLOAT3> shapeDeltas;                      //�e���_�̃V�F�C�v�L�[�K����̈ʒu���i�[ (���_�̐� * �V�F�C�v�L�[�̐������v�f�����m��)
        std::vector<float> shapeWeights;                        //���b�V�����Ƃ̃V�F�C�v�L�[�̃E�F�C�g�ꗗ
        UINT vertexCount;                                       //���b�V���̒��_��
    };

    //�{�[�������݂���FBX�t�@�C�������[�h
    void LoadFBX(const std::string& fbxFile);

    void ProcessNode(const aiScene* scene, aiNode* node);   //�m�[�h��ǂݍ���
    Mesh* ProcessMesh(const aiScene* scene, aiMesh* mesh, HumanoidMesh& humanoidMesh);                       //���b�V������ǂݍ���
    void LoadBones(const aiScene* scene, aiMesh* mesh, std::vector<Vertex>& vertices);    //�{�[�������擾
    void LoadBoneFamily(const aiNode* node);                //�{�[���̐e�q�֌W���擾
    void LoadShapeKey(const aiMesh* mesh, std::vector<Vertex>& vertices, HumanoidMesh& humanoidMesh);       //�V�F�C�v�L�[�ƒ��_�̊֌W���擾
    void UpdateBoneTransform(UINT boneIndex, XMMATRIX& parentMatrix);   //�V�F�[�_�[�ɓn���{�[���̍��W���v�Z
    void UpdateBoneTransform();                                         //�V�F�[�_�[�ɓn���{�[���̍��W���v�Z
    void UpdateShapeKeys();     //�V�F�C�v�L�[�̃E�F�C�g���X�V
    void UpdateAnimation();     //�A�j���[�V�������X�V
    void CreateBuffer(Mesh* pMesh, std::vector<Vertex>& vertices, std::vector<UINT>& indices, HumanoidMesh& humanoidMesh);
    void CreateShapeDeltasTexture(HumanoidMesh& humanoidMesh);

private:  //�v���C�x�[�g�ϐ�
    std::vector<Bone> m_bones;                              //�{�[�����
    std::vector<XMMATRIX> m_boneInfos;                      //�V�F�[�_�[�ɑ��M����{�[���̃}�g���b�N�X
    std::unordered_map<std::string, UINT> m_boneMapping;    //�{�[��������C���f�b�N�X���擾
    std::vector<HumanoidMesh> m_humanoidMeshes;             //�q���[�}�m�C�h�p�̃��b�V�����

    Animation* m_pAnimation;                                //�A�j���[�V�������
    float m_nowAnimationTime;                               //���݂̃A�j���[�V��������
};