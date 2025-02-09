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

//ファイルから8バイトだけ読み取り、double型に入れる
double BinaryReader::ReadDouble()
{
	return Read<double>();
}
//ファイルから4バイトだけ読み取り、float型に入れる
float BinaryReader::ReadFloat()
{
	return Read<float>();
}
//ファイルから4バイトだけ読み取り、int型に入れる
int BinaryReader::ReadInt32()
{
	return Read<int>();
}
//ファイルから4バイトだけ読み取り、unsigned int型に入れる
UINT BinaryReader::ReadUInt32()
{
	return Read<UINT>();
}
//ファイルから2バイトだけ読み取り、unsigned short型に入れる
USHORT BinaryReader::ReadUInt16()
{
	return Read<USHORT>();
}
//ファイルから2バイトだけ読み取り、short型に入れる
short BinaryReader::ReadInt16()
{
	return Read<short>();
}
//ファイルから1バイトだけ読み取り、unsigned char型に入れる
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
//ファイルからreadsizeだけ読み取り、char*型に入れる
char* BinaryReader::ReadBytes(UINT readSize)
{
	std::vector<char> tempBuffer;
	size_t remaining = bufferSize - bufferIndex;

	// まず、バッファ内にある分だけコピー
	if (remaining > 0) {
		size_t copySize = min(remaining, static_cast<size_t>(readSize));
		tempBuffer.insert(tempBuffer.end(), buffer.begin() + bufferIndex, buffer.begin() + bufferIndex + copySize);
		bufferIndex += copySize;
	}

	// 必要なデータがまだ足りない場合、バッファを補充しながらループ
	while (tempBuffer.size() < readSize) {
		if (bMemory) {
			char* temp = new char[1];
			return temp;
		}

		if (!FillBuffer()) {
			break; // バッファ補充が失敗（ファイル終端等）
		}

		size_t toRead = min(bufferSize, readSize - tempBuffer.size());
		tempBuffer.insert(tempBuffer.end(), buffer.begin(), buffer.begin() + toRead);
		bufferIndex = toRead; // 読み取った分だけバッファ位置を更新
	}

	// 読み取れたサイズに応じてメモリ確保
	size_t actualSize = tempBuffer.size();
	char* result = new char[actualSize + 1];
	std::memcpy(result, tempBuffer.data(), actualSize);
	result[actualSize] = '\0';

	return result;
}
//ファイルからreadsizeだけ読み取り、unsigned char*型に入れる
BYTE* BinaryReader::ReadUBytes(int readsize)
{
	if (bufferIndex + readsize > bufferSize) {
		if (bMemory) {
			BYTE* temp = new BYTE[1];
			return temp;
		}

		//残りのデータが足りない場合はバッファを補充
		size_t remaining = bufferSize - bufferIndex;
		std::vector<uint8_t> tempBuffer(readsize);
		std::memcpy(tempBuffer.data(), &buffer[bufferIndex], remaining);

		if (!FillBuffer()) {
			//バッファ補充失敗 -> 読み取れる分だけ返す
			BYTE* partialData = new BYTE[remaining];
			std::memcpy(partialData, tempBuffer.data(), remaining);
			bufferIndex = bufferSize; //末端
			return partialData;
		}

		//残りデータと補充後のデータを統合
		std::memcpy(tempBuffer.data() + remaining, buffer.data(), readsize - remaining);
		BYTE* result = new BYTE[readsize];
		std::memcpy(result, tempBuffer.data(), readsize);
		bufferIndex = readsize - remaining; //新しいバッファの位置を調整
		return result;
	}

	//バッファ内で収まる場合
	BYTE* result = new BYTE[readsize];
	std::memcpy(result, &buffer[bufferIndex], readsize);
	bufferIndex += readsize;
	return result;
}
//現在の位置からseekLengthバイトだけ進める
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

		//バッファを補充
		if (!FillBuffer()) {
			return;
		}

		//インデックスを更新
		bufferIndex = seekLength - bytesRemaining;
	}
	else {
		bufferIndex += seekLength;
	}
}
//バイナリファイルを閉じる
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
		return false; //ファイルの終端
	}

	bufferIndex = 0;
	bufferSize = bytesRead;
	return true;
}
