#pragma once
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <DirectXMath.h>

//�t�@�C���Ƀo�C�i���f�[�^���������ރN���X

class BinaryWriter
{
public:
	//�o�͐���w��
	//���� : const std::string �o�͐�, bool �ǋL���邩�ǂ���
	bool OpenWrite(const std::string filePath, bool bAppend = false);
	bool OpenWrite();

	//����ɊJ����Ă��邩
	bool IsOpened();

	//�v���~�e�B�u�ϐ���ۑ�
    template<typename T>
	void Write(const T& value)
	{
		if (bMemory) {
			memoryStream.write(reinterpret_cast<const char*>(&value), sizeof(T));
		}
		else {
			outputStream.write(reinterpret_cast<const char*>(&value), sizeof(T));
		}
	}

	//�������ۑ�
	void Write(const std::string& value);
	//�w�肵���T�C�Y�̃o�b�t�@��ۑ�
	void Write(const char* buffer, size_t size);
	//�}�g���b�N�X��ۑ�
	void Write(const DirectX::XMMATRIX& matrix);
	//XMFLOAT3��ۑ�
	void Write(const DirectX::XMFLOAT3& value);
	//1�o�C�g������ۑ�
	void Write(const unsigned char byte);
	//bool�^��ۑ�
	void Write(const bool b);

	std::vector<char> GetBuffer() const;

	size_t GetNowPointer();

	void Close();

private:
	std::ofstream outputStream;			//�t�@�C���o�͗p
	std::ostringstream memoryStream;	//�������o�͗p

	bool bMemory = false;
};
