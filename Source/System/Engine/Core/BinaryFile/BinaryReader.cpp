#include "BinaryReader.h"

//いろんな型のバイト数
constexpr DWORD DOUBLE64BYTE = 8;	//doubleやlong longなど
constexpr DWORD INT32BYTE = 4;		//intやfloatなど
constexpr DWORD INT16BYTE = 2;		//主にshort
constexpr DWORD INT8BYTE = 1;		//主にchar

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

//ファイルから8バイトだけ読み取り、double型に入れる
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
//ファイルから4バイトだけ読み取り、float型に入れる
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
//ファイルから4バイトだけ読み取り、int型に入れる
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
//ファイルから4バイトだけ読み取り、unsigned int型に入れる
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
//ファイルから2バイトだけ読み取り、unsigned short型に入れる
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
//ファイルから2バイトだけ読み取り、short型に入れる
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
//ファイルから1バイトだけ読み取り、unsigned char型に入れる
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
//ファイルからreadsizeだけ読み取り、char*型に入れる
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
//ファイルからreadsizeだけ読み取り、unsigned char*型に入れる
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
//指定した位置からseekLengthバイトだけ進める
unsigned long BinaryReader::Seek(SetSeek seekMode, int seekLength) const
{
	if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE) {
		return 0;
	}
	return SetFilePointer(hFile, seekLength, NULL, seekMode);
}
//バイナリファイルを閉じる
void BinaryReader::Close() const
{
	if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE) {
		return;
	}
	CloseHandle(hFile);
}
