#include "BinaryReader.h"

//�����Ȍ^�̃o�C�g��
constexpr DWORD DOUBLE64BYTE = 8;	//double��long long�Ȃ�
constexpr DWORD INT32BYTE = 4;		//int��float�Ȃ�
constexpr DWORD INT16BYTE = 2;		//���short
constexpr DWORD INT8BYTE = 1;		//���char

BinaryReader::BinaryReader(std::string filePath)
{
	hFile = CreateFileA(filePath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		printf("aaaaa\n");
		return;
	}
}

BinaryReader::~BinaryReader()
{
	if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE) {
		return;
	}
	Close();
}

//�t�@�C������8�o�C�g�����ǂݎ��Adouble�^�ɓ����
double BinaryReader::ReadDouble() const
{
	if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE) {
		return 0.0;
	}
	char buf[DOUBLE64BYTE]{};
	DWORD dwActualRead;
	int rc = ReadFile(hFile, buf, DOUBLE64BYTE, &dwActualRead, NULL);
	if (rc == 0 || DOUBLE64BYTE != dwActualRead) {
		return 0.0;
	}
	double double64;
	memcpy(&double64, &buf, DOUBLE64BYTE);
	return double64;
}
//�t�@�C������4�o�C�g�����ǂݎ��Afloat�^�ɓ����
float BinaryReader::ReadFloat() const
{
	if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE) {
		return 0.0f;
	}
	char buf[INT32BYTE]{};
	DWORD dwActualRead;
	int rc = ReadFile(hFile, buf, INT32BYTE, &dwActualRead, NULL);
	if (rc == 0 || INT32BYTE != dwActualRead) {
		return 0.0f;
	}
	float float32;
	memcpy(&float32, &buf, INT32BYTE);
	return float32;
}
//�t�@�C������4�o�C�g�����ǂݎ��Aint�^�ɓ����
int BinaryReader::ReadInt32() const
{
	if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE) {
		return -1;
	}
	char buf[INT32BYTE]{};
	DWORD dwActualRead;
	int rc = ReadFile(hFile, buf, INT32BYTE, &dwActualRead, NULL);
	if (rc == 0 || INT32BYTE != dwActualRead) {
		return -1;
	}
	int int32;
	memcpy(&int32, &buf, INT32BYTE);
	return int32;
}
//�t�@�C������4�o�C�g�����ǂݎ��Aunsigned int�^�ɓ����
unsigned int BinaryReader::ReadUInt32() const
{
	if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE) {
		return 0;
	}
	char buf[INT32BYTE]{};
	DWORD dwActualRead;
	int rc = ReadFile(hFile, buf, INT32BYTE, &dwActualRead, NULL);
	if (rc == 0 || INT32BYTE != dwActualRead) {
		return 0;
	}
	unsigned int uint32;
	memcpy(&uint32, &buf, INT32BYTE);
	return uint32;
}
//�t�@�C������2�o�C�g�����ǂݎ��Aunsigned short�^�ɓ����
unsigned short BinaryReader::ReadUInt16() const
{
	if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE) {
		return 0;
	}
	char buf[INT16BYTE]{};
	DWORD dwActualRead;
	int rc = ReadFile(hFile, buf, INT16BYTE, &dwActualRead, NULL);
	if (rc == 0 || INT16BYTE != dwActualRead) {
		return 0;
	}
	unsigned short int16;
	memcpy(&int16, &buf, INT16BYTE);
	return int16;
}
//�t�@�C������2�o�C�g�����ǂݎ��Ashort�^�ɓ����
short BinaryReader::ReadInt16() const
{
	if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE) {
		return -1;
	}
	char buf[INT16BYTE]{};
	DWORD dwActualRead;
	int rc = ReadFile(hFile, buf, INT16BYTE, &dwActualRead, NULL);
	if (rc == 0 || INT16BYTE != dwActualRead) {
		return -1;
	}
	short int16;
	memcpy(&int16, &buf, INT16BYTE);
	return int16;
}
//�t�@�C������1�o�C�g�����ǂݎ��Aunsigned char�^�ɓ����
unsigned char BinaryReader::ReadByte() const
{
	if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE) {
		return 0;
	}
	char buf[1]{};
	DWORD dwActualRead;
	int rc = ReadFile(hFile, buf, INT8BYTE, &dwActualRead, NULL);
	if (rc == 0 || INT8BYTE != dwActualRead) {
		return 0;
	}
	unsigned char int8;
	memcpy(&int8, &buf, INT8BYTE);
	return int8;
}
//�t�@�C������readsize�����ǂݎ��Achar*�^�ɓ����
char* BinaryReader::ReadBytes(int readsize) const
{
	if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE) {
		return nullptr;
	}
	char* buf = (char*)malloc(static_cast<size_t>(readsize) + 1);
	DWORD dwActualRead;
	int rc = ReadFile(hFile, buf, readsize, &dwActualRead, NULL);
	if (rc == 0 || readsize != (int)dwActualRead) {
		return nullptr;
	}
	buf[readsize] = '\0';
	return buf;
}
//�t�@�C������readsize�����ǂݎ��Aunsigned char*�^�ɓ����
unsigned char* BinaryReader::ReadUBytes(int readsize) const
{
	if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE) {
		return nullptr;
	}
	unsigned char* buf = (unsigned char*)malloc(readsize);
	DWORD dwActualRead;
	int rc = ReadFile(hFile, buf, readsize, &dwActualRead, NULL);
	if (rc == 0 || readsize != (int)dwActualRead) {
		return nullptr;
	}
	return buf;
}
//�w�肵���ʒu����seekLength�o�C�g�����i�߂�
unsigned long BinaryReader::Seek(SetSeek seekMode, int seekLength) const
{
	if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE) {
		return 0;
	}
	return SetFilePointer(hFile, seekLength, NULL, seekMode);
}
//�o�C�i���t�@�C�������
void BinaryReader::Close() const
{
	if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE) {
		return;
	}
	CloseHandle(hFile);
}
