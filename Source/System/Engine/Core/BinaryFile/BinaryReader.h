#pragma once
#include <Windows.h>
#include <string>

class BinaryReader
{
public:
	enum SetSeek
	{
		Begin,
		Current,
		End
	};

	//�R���X�g���N�^
	//���� : �t�@�C���p�X
	BinaryReader(std::string filePath);

	~BinaryReader();

	//�t�@�C������8�o�C�g�����ǂݎ��Adouble�^�ɓ����
	double ReadDouble() const;

	//�t�@�C������4�o�C�g�����ǂݎ��Afloat�^�ɓ����
	float ReadFloat() const;

	//�t�@�C������4�o�C�g�����ǂݎ��Aint�^�ɓ����
	int ReadInt32() const;

	//�t�@�C������4�o�C�g�����ǂݎ��Aunsigned int�^�ɓ����
	unsigned int ReadUInt32() const;

	//�t�@�C������2�o�C�g�����ǂݎ��Aunsigned short�^�ɓ����
	unsigned short ReadUInt16() const;

	//�t�@�C������2�o�C�g�����ǂݎ��Ashort�^�ɓ����
	short ReadInt16() const;

	//�t�@�C������1�o�C�g�����ǂݎ��Aunsigned char�^�ɓ����
	unsigned char ReadByte() const;

	//�t�@�C������readsize�����ǂݎ��Achar*�^�ɓ����
	//���� : �ǂݎ��T�C�Y (�o�C�g�P��)
	char* ReadBytes(int readSize) const;

	//�t�@�C������readsize�����ǂݎ��Aunsigned char*�^�ɓ����
	//���� : �ǂݎ��T�C�Y (�o�C�g�P��)
	unsigned char* ReadUBytes(int readSize) const;

	//�w�肵���ʒu����seekLength�o�C�g�����i�߂�
	//���� ; SetSeek(�V�[�N�ꏊ), �X�L�b�v����T�C�Y (�o�C�g�P��)
	unsigned long Seek(SetSeek seekMode, int seekLength) const;

	//�o�C�i���t�@�C�������
	void Close() const;

private:
	HANDLE hFile;
};