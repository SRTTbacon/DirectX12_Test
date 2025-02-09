#include "BinaryReader.h"

BinaryReader::BinaryReader(const std::string& filePath)
	: bMemory(false)
{
	hFile = CreateFileA(filePath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		return;
	}

	buffer.resize(DEFAULT_BUFFER_SIZE);
	FillBuffer();
}

BinaryReader::BinaryReader(std::vector<char>& buffer)
	: bMemory(true)
	, hFile(nullptr)
{
	this->buffer = buffer;
	bufferSize = buffer.size();
	bufferIndex = 0;
}

BinaryReader::~BinaryReader()
{
	if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE) {
		return;
	}
	Close();
}

DirectX::XMMATRIX BinaryReader::ReadMatrix()
{
	DirectX::XMMATRIX matrix = DirectX::XMMatrixIdentity();
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			matrix.r[i].m128_f32[j] = ReadFloat();
		}
	}

	return matrix;
}

//�t�@�C������8�o�C�g�����ǂݎ��Adouble�^�ɓ����
double BinaryReader::ReadDouble()
{
	return Read<double>();
}
//�t�@�C������4�o�C�g�����ǂݎ��Afloat�^�ɓ����
float BinaryReader::ReadFloat()
{
	return Read<float>();
}
//�t�@�C������4�o�C�g�����ǂݎ��Aint�^�ɓ����
int BinaryReader::ReadInt32()
{
	return Read<int>();
}
//�t�@�C������4�o�C�g�����ǂݎ��Aunsigned int�^�ɓ����
UINT BinaryReader::ReadUInt32()
{
	return Read<UINT>();
}
//�t�@�C������2�o�C�g�����ǂݎ��Aunsigned short�^�ɓ����
USHORT BinaryReader::ReadUInt16()
{
	return Read<USHORT>();
}
//�t�@�C������2�o�C�g�����ǂݎ��Ashort�^�ɓ����
short BinaryReader::ReadInt16()
{
	return Read<short>();
}
//�t�@�C������1�o�C�g�����ǂݎ��Aunsigned char�^�ɓ����
unsigned char BinaryReader::ReadByte()
{
	if (bufferIndex >= bufferSize) {
		if (!FillBuffer()) {
			return 0x00;
		}
	}
	return buffer[bufferIndex++];
}
char BinaryReader::ReadSByte()
{
	if (bufferIndex >= bufferSize) {
		if (!FillBuffer()) {
			return 0x00;
		}
	}
	return (char)buffer[bufferIndex++];
}
bool BinaryReader::ReadBoolean()
{
	if (bufferIndex >= bufferSize) {
		if (!FillBuffer()) {
			return false;
		}
	}
	return buffer[bufferIndex++] != 0;
}
//�t�@�C������readsize�����ǂݎ��Achar*�^�ɓ����
char* BinaryReader::ReadBytes(UINT readSize)
{
	std::vector<char> tempBuffer;
	size_t remaining = bufferSize - bufferIndex;

	// �܂��A�o�b�t�@���ɂ��镪�����R�s�[
	if (remaining > 0) {
		size_t copySize = min(remaining, static_cast<size_t>(readSize));
		tempBuffer.insert(tempBuffer.end(), buffer.begin() + bufferIndex, buffer.begin() + bufferIndex + copySize);
		bufferIndex += copySize;
	}

	// �K�v�ȃf�[�^���܂�����Ȃ��ꍇ�A�o�b�t�@���[���Ȃ��烋�[�v
	while (tempBuffer.size() < readSize) {
		if (bMemory) {
			char* temp = new char[1];
			return temp;
		}

		if (!FillBuffer()) {
			break; // �o�b�t�@��[�����s�i�t�@�C���I�[���j
		}

		size_t toRead = min(bufferSize, readSize - tempBuffer.size());
		tempBuffer.insert(tempBuffer.end(), buffer.begin(), buffer.begin() + toRead);
		bufferIndex = toRead; // �ǂݎ�����������o�b�t�@�ʒu���X�V
	}

	// �ǂݎ�ꂽ�T�C�Y�ɉ����ă������m��
	size_t actualSize = tempBuffer.size();
	char* result = new char[actualSize + 1];
	std::memcpy(result, tempBuffer.data(), actualSize);
	result[actualSize] = '\0';

	return result;
}
//�t�@�C������readsize�����ǂݎ��Aunsigned char*�^�ɓ����
BYTE* BinaryReader::ReadUBytes(int readsize)
{
	if (bufferIndex + readsize > bufferSize) {
		if (bMemory) {
			BYTE* temp = new BYTE[1];
			return temp;
		}

		//�c��̃f�[�^������Ȃ��ꍇ�̓o�b�t�@���[
		size_t remaining = bufferSize - bufferIndex;
		std::vector<uint8_t> tempBuffer(readsize);
		std::memcpy(tempBuffer.data(), &buffer[bufferIndex], remaining);

		if (!FillBuffer()) {
			//�o�b�t�@��[���s -> �ǂݎ��镪�����Ԃ�
			BYTE* partialData = new BYTE[remaining];
			std::memcpy(partialData, tempBuffer.data(), remaining);
			bufferIndex = bufferSize; //���[
			return partialData;
		}

		//�c��f�[�^�ƕ�[��̃f�[�^�𓝍�
		std::memcpy(tempBuffer.data() + remaining, buffer.data(), readsize - remaining);
		BYTE* result = new BYTE[readsize];
		std::memcpy(result, tempBuffer.data(), readsize);
		bufferIndex = readsize - remaining; //�V�����o�b�t�@�̈ʒu�𒲐�
		return result;
	}

	//�o�b�t�@���Ŏ��܂�ꍇ
	BYTE* result = new BYTE[readsize];
	std::memcpy(result, &buffer[bufferIndex], readsize);
	bufferIndex += readsize;
	return result;
}
//���݂̈ʒu����seekLength�o�C�g�����i�߂�
void BinaryReader::Seek(UINT seekLength)
{
	if (hFile == INVALID_HANDLE_VALUE && !bMemory) {
		return;
	}

	size_t bytesRemaining = bufferSize - bufferIndex;

	if (bytesRemaining < seekLength) {
		if (bMemory) {
			return;
		}

		//�o�b�t�@���[
		if (!FillBuffer()) {
			return;
		}

		//�C���f�b�N�X���X�V
		bufferIndex = seekLength - bytesRemaining;
	}
	else {
		bufferIndex += seekLength;
	}
}
//�o�C�i���t�@�C�������
void BinaryReader::Close()
{
	if (hFile == INVALID_HANDLE_VALUE || bMemory) {
		return;
	}
	CloseHandle(hFile);
	hFile = nullptr;
}

bool BinaryReader::FillBuffer()
{
	if (hFile == INVALID_HANDLE_VALUE || bMemory) {
		return false;
	}

	DWORD bytesRead = 0;
	BOOL success = ReadFile(hFile, buffer.data(), DEFAULT_BUFFER_SIZE, &bytesRead, NULL);
	if (!success || bytesRead == 0) {
		printf("ErrorReadFile\n");
		return false; //�t�@�C���̏I�[
	}

	bufferIndex = 0;
	bufferSize = bytesRead;
	return true;
}
