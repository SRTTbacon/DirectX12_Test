#pragma once
#include "Model.h"

class Character : public Model
{
public:
	Character(const std::string fbxFile, const Camera* pCamera);

    void Update();

    //�{�[���̈ʒu���X�V
    void UpdateBonePosition(std::string boneName, XMFLOAT3& position);
    //�{�[���̉�]��ύX
    void UpdateBoneRotation(std::string boneName, XMFLOAT3& rotation);
    //�{�[���̃X�P�[����ύX
    void UpdateBoneScale(std::string boneName, XMFLOAT3& scale);

    std::vector<std::string> GetBoneNames();

    inline Bone* GetBone(std::string boneName)
    {
        if (boneMapping.find(boneName) == boneMapping.end()) {
            return nullptr;
        }
        return &bones[boneMapping[boneName]];
    }
    inline XMFLOAT3 GetBoneOffset(std::string boneName)
    {
        XMMATRIX m = bones[boneMapping[boneName]].GetBoneOffset();
        return XMFLOAT3(m.r[3].m128_f32[0], m.r[3].m128_f32[1], m.r[3].m128_f32[2]);
    }

private:
    //�V�F�[�_�[�ɓn�����_���
    struct Vertex {
        XMFLOAT3 Position;
        XMFLOAT3 Normal;
        XMFLOAT2 TexCoords;
        XMFLOAT4 Color; // ���_�F
        XMFLOAT4 BoneWeights;
        UINT BoneIDs[4];
    };

    //�{�[�������݂���FBX�t�@�C�������[�h
    void LoadFBX(const std::string& fbxFile);

    void ProcessNode(const aiScene* scene, aiNode* node);   //�m�[�h��ǂݍ���
    Mesh ProcessMesh(const aiScene* scene, aiMesh* mesh);   //���b�V������ǂݍ���
    void LoadBones(const aiScene* scene, Mesh& meshStruct, aiMesh* mesh, std::vector<Vertex>& vertices);    //�{�[�������擾
    void LoadBoneFamily(const aiNode* node);                //�{�[���̐e�q�֌W���擾
    void UpdateBoneTransform(UINT boneIndex, XMMATRIX& parentMatrix);   //�V�F�[�_�[�ɓn���{�[���̍��W���v�Z
    void UpdateBoneTransform();                                         //�V�F�[�_�[�ɓn���{�[���̍��W���v�Z
};