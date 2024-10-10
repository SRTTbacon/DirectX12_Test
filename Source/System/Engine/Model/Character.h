#pragma once
#include "Model.h"
#include "..\\..\\Main\\Utility.h"

class Character : public Model
{
public:
	Character(const std::string fbxFile, const Camera* pCamera);

    void Update();

    //�{�[���̈ʒu���X�V
    void UpdateBonePosition(std::string boneName, XMFLOAT3& position);
    //�{�[���̉�]��ύX
    void UpdateBoneRotation(std::string boneName, XMFLOAT4& rotation);
    //�{�[���̃X�P�[����ύX
    void UpdateBoneScale(std::string boneName, XMFLOAT3& scale);

    //�V�F�C�v�L�[�̃E�F�C�g���X�V
    void SetShapeWeight(UINT shapeIndex, float weight);
    void SetShapeWeight(const std::string shapeName, float weight);

public: //�Q�b�^�[�֐� (�p�ɂɌĂяo�����̂�inline)

    //���ׂẴ{�[�������擾
    std::vector<std::string> GetBoneNames();

    //�{�[�������擾
    inline Bone* GetBone(std::string boneName)
    {
        if (boneMapping.find(boneName) == boneMapping.end()) {
            return nullptr;
        }
        return &bones[boneMapping[boneName]];
    }
    //�{�[���̃I�t�Z�b�g���擾
    inline XMFLOAT3 GetBoneOffset(std::string boneName)
    {
        if (boneMapping.find(boneName) == boneMapping.end()) {
            return XMFLOAT3(0.0f, 0.0f, 0.0f);
        }

        XMMATRIX m = bones[boneMapping[boneName]].GetBoneOffset();
        return XMFLOAT3(m.r[3].m128_f32[0], m.r[3].m128_f32[1], m.r[3].m128_f32[2]);
    }

    //�V�F�C�v�L�[�̃E�F�C�g���擾
    inline float GetShapeWeight(std::string shapeName)
    {
        if (shapeMapping.find(shapeName) == shapeMapping.end())
            return 0.0f;

        return GetShapeWeight(shapeMapping[shapeName]);
    }
    //�V�F�C�v�L�[�̃E�F�C�g���擾
    inline float GetShapeWeight(UINT shapeIndex)
    {
        if (shapeIndex < 0 || shapeWeights.size() <= shapeIndex)
            return 0.0f;

        return shapeWeights[shapeIndex];
    }

private:
    //�v���C�x�[�g�ϐ�
    std::vector<Bone> bones;                            //�{�[�����
    std::unordered_map<std::string, UINT> boneMapping;  //�{�[��������C���f�b�N�X���擾

    std::unordered_map<std::string, UINT> shapeMapping; //�V�F�C�v�L�[������C���f�b�N�X���擾

private:
    //�V�F�[�_�[�ɓn�����_���
    struct Vertex {
        XMFLOAT3 Position;
        XMFLOAT3 Normal;
        XMFLOAT2 TexCoords;
        XMFLOAT4 Color; // ���_�F
        XMFLOAT4 BoneWeights;
        UINT BoneIDs[4];
        XMFLOAT3 ShapePosition;
        UINT ShapeID;
    };

    //�{�[�������݂���FBX�t�@�C�������[�h
    void LoadFBX(const std::string& fbxFile);

    void ProcessNode(const aiScene* scene, aiNode* node);   //�m�[�h��ǂݍ���
    Mesh ProcessMesh(const aiScene* scene, aiMesh* mesh);   //���b�V������ǂݍ���
    void LoadBones(const aiScene* scene, Mesh& meshStruct, aiMesh* mesh, std::vector<Vertex>& vertices);    //�{�[�������擾
    void LoadBoneFamily(const aiNode* node);                //�{�[���̐e�q�֌W���擾
    void LoadShapeKey(const aiMesh* mesh, std::vector<Vertex>& vertices);   //�V�F�C�v�L�[�ƒ��_�̊֌W���擾
    void UpdateBoneTransform(UINT boneIndex, XMMATRIX& parentMatrix);   //�V�F�[�_�[�ɓn���{�[���̍��W���v�Z
    void UpdateBoneTransform();                                         //�V�F�[�_�[�ɓn���{�[���̍��W���v�Z
    void UpdateShapeKeys();
};