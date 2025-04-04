#pragma once
#include <DirectXMath.h>
#include <Windows.h>
#include <string>
#include <vector>

class BinaryReader
{
public:
	enum SeekType
	{
		Begin,
		Current,
		End
	};

	//�R���X�g���N�^
	//���� : �t�@�C���p�X
	BinaryReader(const std::string& filePath, UINT maxReadKB = 1024 * 10);
	BinaryReader(std::vector<char>& buffer, UINT maxReadKB = 1024 * 10);

	~BinaryReader();

	//�t�@�C������128�o�C�g�����ǂݎ��AXMMATRIX�^�ɓ����
	DirectX::XMMATRIX ReadMatrix();

	DirectX::XMFLOAT2 ReadFloat2();

	//�t�@�C������12�o�C�g�����ǂݎ��AXMFLOAT3�^�ɓ����
	DirectX::XMFLOAT3 ReadFloat3();

	//�t�@�C������8�o�C�g�����ǂݎ��Adouble�^�ɓ����
	double ReadDouble();

	//�t�@�C������4�o�C�g�����ǂݎ��Afloat�^�ɓ����
	float ReadFloat();

	//�t�@�C������4�o�C�g�����ǂݎ��Aint�^�ɓ����
	int ReadInt32();

	//�t�@�C������4�o�C�g�����ǂݎ��Aunsigned int�^�ɓ����
	UINT ReadUInt32();

	//�t�@�C������2�o�C�g�����ǂݎ��Aunsigned short�^�ɓ����
	USHORT ReadUInt16();

	//�t�@�C������2�o�C�g�����ǂݎ��Ashort�^�ɓ����
	short ReadInt16();

	//�t�@�C������1�o�C�g�����ǂݎ��Aunsigned char�^�ɓ����
	BYTE ReadByte();
	//�t�@�C������1�o�C�g�����ǂݎ��Achar�^�ɓ����
	char ReadSByte();

	//�t�@�C������1�o�C�g�����ǂݎ��Abool�^�ɓ����
	bool ReadBoolean();

	//�t�@�C������readsize�����ǂݎ��Achar*�^�ɓ����
	//���� : �ǂݎ��T�C�Y (�o�C�g�P��)
	char* ReadBytes(UINT readSize);

	//�t�@�C������readsize�����ǂݎ��Aunsigned char*�^�ɓ����
	//���� : �ǂݎ��T�C�Y (�o�C�g�P��)
	BYTE* ReadUBytes(int readSize);

	//�w�肵���ʒu����seekLength�o�C�g�����i�߂�
	//���� ; �X�L�b�v����T�C�Y (�o�C�g�P��)
	void Seek(UINT seekLength, SeekType seekType);

	//�o�C�i���t�@�C�������
	void Close();

public:
	inline bool GetIsOpen() const { return (hFile != nullptr && hFile != INVALID_HANDLE_VALUE); }

private:
	HANDLE hFile;

	std::vector<char> buffer;
	size_t bufferIndex = 0;
	size_t bufferSize = 0;
	bool bMemory;
	UINT readBufferSize = 1024 * 1024 * 10; //10MB���ǂݎ��

	//1�x��1024KB�t�@�C������ǂݎ��A�������ɕۑ�
	bool FillBuffer();

	template <typename T>
	T Read();
};

template<typename T>
inline T BinaryReader::Read()
{
	const size_t typeSize = sizeof(T);
	size_t bytesRemaining = bufferSize - bufferIndex;

	if (bytesRemaining < typeSize) {
		if (bMemory) {
			if constexpr (std::is_arithmetic<T>::value) {
				return T(0); //�Z�p�^�̏ꍇ�̓[����Ԃ�
			}
			else {
				return T();  //���̑��̌^�̏ꍇ�̓f�t�H���g�l��Ԃ�
			}
		}

		//�o�b�t�@�Ɏc���Ă���f�[�^���ꎞ�ۑ�
		uint8_t tempBuffer[typeSize] = { 0 };

		if (bytesRemaining > 0) {
			std::memcpy(tempBuffer, &buffer[bufferIndex], bytesRemaining);
		}

		//�o�b�t�@���[
		if (!FillBuffer()) {
			if constexpr (std::is_arithmetic<T>::value) {
				return T(0); //�Z�p�^�̏ꍇ�̓[����Ԃ�
			}
			else {
				return T();  //���̑��̌^�̏ꍇ�̓f�t�H���g�l��Ԃ�
			}
		}

		//��[��̃f�[�^�𓝍�
		std::memcpy(tempBuffer + bytesRemaining, buffer.data(), typeSize - bytesRemaining);

		//�C���f�b�N�X���X�V
		bufferIndex = typeSize - bytesRemaining;

		//�l���쐬���ĕԂ�
		T value{};
		std::memcpy(&value, tempBuffer, typeSize);
		return value;
	}

	//�o�b�t�@���Ŏ��܂�ꍇ
	T value{};
	std::memcpy(&value, &buffer[bufferIndex], typeSize);
	bufferIndex += typeSize;
	return value;
}
