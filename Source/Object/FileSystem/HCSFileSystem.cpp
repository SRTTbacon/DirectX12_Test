#include "HCSFileSystem.h"

#include "..\\..\\System\\Engine\\Core\\BinaryFile\\BinaryCompression.h"

#include <filesystem>

//------------HCSReadFile------------

HCSReadFile::HCSReadFile()
	: m_pBr(nullptr)
{
}

HCSReadFile::~HCSReadFile()
{
	Close();
}

bool HCSReadFile::Open(std::string strFilePath)
{
	Close();

	m_pBr = new BinaryReader(strFilePath);

	if (!m_pBr->GetIsOpen()) {
		return false;
	}

	char* pHeader = m_pBr->ReadBytes(m_pBr->ReadByte());
	std::string header = pHeader;
	delete pHeader;
	if (header != HCSWriteFile::HCS_FILE_HEADER) {
		Close();
		return false;
	}

	m_pBr->ReadByte();	//�o�[�W����

	UINT bufferOriginalSize = m_pBr->ReadUInt32();
	UINT bufferCompressedSize = m_pBr->ReadUInt32();
	char* compressedBuffer = m_pBr->ReadBytes(bufferCompressedSize);

	//���k����Ă���{�[��������
	std::vector<char> buffer;
	BinaryDecompress(buffer, bufferOriginalSize, compressedBuffer, bufferCompressedSize);

	delete[] compressedBuffer;

	//�𓀂����o�b�t�@��ǂݍ���
	BinaryReader bufferReader(buffer);

	//�t�@�C����
	USHORT fileCount = bufferReader.ReadUInt16();

	for (USHORT i = 0; i < fileCount; i++) {
		//�t�@�C����
		char* pFilePath = bufferReader.ReadBytes(bufferReader.ReadByte());
		std::string filePath = pFilePath;
		delete pFilePath;

		//���X�g�ɒǉ�
		m_filePointers.emplace(filePath, 0);
		m_files.push_back(filePath);
	}

	for (USHORT i = 0; i < fileCount; i++) {
		//�t�@�C���������Ă���ʒu
		UINT filePointer = m_pBr->ReadUInt32();
		m_filePointers[m_files[i]] = filePointer;
	}

	return true;
}

char* HCSReadFile::GetFile(std::string strFilePath, OUT size_t* size)
{
	if (m_filePointers.find(strFilePath) == m_filePointers.end()) {
		printf(".hcs���� %s �����݂��܂���B\n", strFilePath.c_str());
		if (size) {
			*size = 0;
		}
		return nullptr;
	}

	UINT filePointer = m_filePointers[strFilePath];

	m_pBr->Seek(filePointer, BinaryReader::SeekType::Begin);

	UINT readSize = m_pBr->ReadUInt32();
	char* pBuffer = m_pBr->ReadBytes(readSize);

	if (size) {
		*size = readSize;
	}

	return pBuffer;
}

void HCSReadFile::Close()
{
	if (m_pBr) {
		m_pBr->Close();
		delete m_pBr;
		m_pBr = nullptr;
	}
}

//------------HCSWriteFile------------

const std::string HCSWriteFile::HCS_FILE_HEADER = "HCSFile";
const char HCSWriteFile::HCS_FILE_VERSION = 0x00;

HCSWriteFile::HCSWriteFile(std::string strFilePath)
{
	m_bw.OpenWrite(strFilePath);
}

void HCSWriteFile::AddFile(std::string strFilePath)
{
	//���ɒǉ�����Ă���ꍇ�͏I��
	for (std::string file : m_files) {
		if (file == strFilePath) {
			return;
		}
	}

	m_files.push_back(strFilePath);
}

void HCSWriteFile::Execute()
{
	std::vector<UINT> filePointers(m_files.size(), 0);

	//�t�@�C�����������ވʒu���擾
	Execute(filePointers, true);
	//�������݈ʒu���l�����A�G�N�X�|�[�g
	Execute(filePointers, false);
}

void HCSWriteFile::Execute(std::vector<UINT>& filePointers, bool bOut)
{
	BinaryWriter tempBw;
	tempBw.OpenWrite();

	tempBw.Write(HCS_FILE_HEADER);
	tempBw.Write(HCS_FILE_VERSION);

	BinaryWriter tempBw2;
	tempBw2.OpenWrite();

	tempBw2.Write(static_cast<USHORT>(m_files.size()));

	for (size_t i = 0; i < m_files.size(); i++) {
		tempBw2.Write(m_files[i]);
	}

	WriteCompression(tempBw, tempBw2);
	tempBw2.Close();

	for (size_t i = 0; i < m_files.size(); i++) {
		if (!bOut) {
			tempBw.Write(filePointers[i]);
		}
		else {
			tempBw.Write(0);
		}
	}

	for (size_t i = 0; i < m_files.size(); i++) {
		if (std::filesystem::exists(m_files[i])) {
			std::ifstream file(m_files[i], std::ios::binary);
			if (!file) {
				printf("�t�@�C�����J���܂���: %s\n", m_files[i].c_str());
				return;
			}

			std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			if (bOut) {
				filePointers[i] = static_cast<UINT>(tempBw.GetNowPointer());
			}
			tempBw.Write(static_cast<UINT>(buffer.size()));
			tempBw.Write(buffer.data(), buffer.size());
		}
		else {
			printf("�t�@�C�� - %s �����݂��܂���B\n", m_files[i].c_str());
			return;
		}
	}

	if (!bOut) {
		std::vector<char> buffer = tempBw.GetBuffer();
		m_bw.Write(buffer.data(), buffer.size());

		m_bw.Close();
	}
}

void HCSWriteFile::WriteCompression(BinaryWriter& destBw, BinaryWriter& sourBw)
{
	//�f�[�^�����k���ď�������
	std::vector<char> compressedBuffer;
	std::vector<char> sourceBuffer = sourBw.GetBuffer();
	BinaryCompress(sourceBuffer.data(), sourceBuffer.size(), compressedBuffer);

	destBw.Write(static_cast<UINT>(sourceBuffer.size()));			//���k�O�̃T�C�Y (�W�J���Ɍv�Z���鏈�����Ȃ����߂��炩���ߕۑ�)
	destBw.Write(static_cast<UINT>(compressedBuffer.size()));       //���k��̃T�C�Y
	destBw.Write(compressedBuffer.data(), compressedBuffer.size()); //���k��̃f�[�^
}
