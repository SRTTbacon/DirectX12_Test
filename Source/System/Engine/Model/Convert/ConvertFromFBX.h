#pragma once

//.fbx�t�@�C����Ǝ��t�H�[�}�b�g(.hcs)�ɕϊ�
//�K�v�Œ���̏�񂵂��ۑ����Ȃ����߃L�����N�^�[��ϊ������ꍇ�A���[�h����1/4�A�������g�p��3/4�قǂɂȂ�͂�
//�t�@�C���\�����P���Ȃ��߁A�Í����Ȃǂ��K�v�ȏꍇ�R�[�h��ύX���Ă�������
//Zstandard���g�p���Ĉ��k�����Ă��܂� (���[�h���ԂɎx�Ⴊ�łȂ��͈͂�)

#include "..\\Character.h"
#include "..\\..\\Core\\BinaryFile\\BinaryCompression.h"

constexpr const float SAVE_SHAPE_DELTA = 0.001f;
constexpr const int COMPRESSION_LEVEL = 3;		//���k���x�� (1�`22) ���[�h���ԂƑ��k

enum ConvertResult
{
	Success,			//�ϊ�����
	FileNotExist,		//fbx�t�@�C�������݂��Ȃ�
	CantWriteFile,		//�o�͐�ɏ������߂Ȃ�
	Nullptr,			//���f�����ǂݍ��܂�Ă��Ȃ�
	FbxFormatExeption	//fbx�t�@�C�����j�����Ă���
};

class ConvertFromFBX
{
public:
	//.fbx����ǂݍ��܂ꂽ�L�����N�^�[��.hcs�t�@�C���ɕϊ�
	//���� :	1 - Character* ���Ƀ��[�h����Ă���Character�N���X�̃|�C���^
	//	2 - std::string Character�̃��[�h�Ɏg�p����fbx�t�@�C��
	//	3 - std::string �o�͐�̃t�@�C���p�X(�g���q�͂Ԃ����Ⴏ���ł��ǂ�)
	ConvertResult ConvertFromCharacter(Character* pCharacter, std::string fromFilePath, std::string toFilePath);
	ConvertResult ConvertFromModel(std::string fromFilePath, std::string toFilePath);

private:

	BinaryWriter bw;	//�o�C�i���f�[�^�Ƃ��ăt�@�C���ɏ������ނ��߂̃N���X
	std::unordered_map<std::string, UINT> m_boneMapping;    //�{�[��������C���f�b�N�X���擾

	void ProcessNode(const aiScene* pScene, const aiNode* pNode);
	void ProcessMesh(const aiMesh* pMesh, bool bIncludeBone, const aiNode* pNode = nullptr);
	void ProcessBone(Character* pCharacter);
	void ProcessHumanMesh(Character* pCharacter);
	void ProcessAnimation(const aiScene* pScene);

	void WriteCompression(std::vector<char>& originalBuffer);

	DirectX::XMMATRIX GetMeshDefaultMatrix(const aiNode* pNode);
};
