#pragma once

#include "..\\..\System\\Engine\\Core\\BinaryFile\\BinaryReader.h"
#include "..\\..\System\\Engine\\Core\\BinaryFile\\BinaryWriter.h"

#include <unordered_map>

class HCSReadFile
{
public:
	HCSReadFile();
	~HCSReadFile();

	bool Open(std::string strFilePath);

	char* GetFile(std::string strFilePath, OUT size_t* size);

	void Close();

public:
	inline UINT GetFileCount() const { return static_cast<UINT>(m_files.size()); }

private:
	BinaryReader* m_pBr;

	std::unordered_map<std::string, UINT> m_filePointers;
	std::vector<std::string> m_files;
};

class HCSWriteFile
{
public:
	HCSWriteFile(std::string strFilePath);

	void AddFile(std::string strFilePath);

	void Execute();

public:
	static const std::string HCS_FILE_HEADER;
	static const char HCS_FILE_VERSION;

private:
	std::vector<std::string> m_files;

	BinaryWriter m_bw;

	void Execute(std::vector<UINT>& filePointers, bool bOut);

	void WriteCompression(BinaryWriter& destBw, BinaryWriter& sourBw);
};
