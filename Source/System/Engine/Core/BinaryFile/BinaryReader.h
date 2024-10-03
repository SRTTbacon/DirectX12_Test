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

	//コンストラクタ
	//引数 : ファイルパス
	BinaryReader(std::string filePath);

	~BinaryReader();

	//ファイルから8バイトだけ読み取り、double型に入れる
	double ReadDouble() const;

	//ファイルから4バイトだけ読み取り、float型に入れる
	float ReadFloat() const;

	//ファイルから4バイトだけ読み取り、int型に入れる
	int ReadInt32() const;

	//ファイルから4バイトだけ読み取り、unsigned int型に入れる
	unsigned int ReadUInt32() const;

	//ファイルから2バイトだけ読み取り、unsigned short型に入れる
	unsigned short ReadUInt16() const;

	//ファイルから2バイトだけ読み取り、short型に入れる
	short ReadInt16() const;

	//ファイルから1バイトだけ読み取り、unsigned char型に入れる
	unsigned char ReadByte() const;

	//ファイルからreadsizeだけ読み取り、char*型に入れる
	//引数 : 読み取るサイズ (バイト単位)
	char* ReadBytes(int readSize) const;

	//ファイルからreadsizeだけ読み取り、unsigned char*型に入れる
	//引数 : 読み取るサイズ (バイト単位)
	unsigned char* ReadUBytes(int readSize) const;

	//指定した位置からseekLengthバイトだけ進める
	//引数 ; SetSeek(シーク場所), スキップするサイズ (バイト単位)
	unsigned long Seek(SetSeek seekMode, int seekLength) const;

	//バイナリファイルを閉じる
	void Close() const;

private:
	HANDLE hFile;
};