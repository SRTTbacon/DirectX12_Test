#pragma once
#include <zstd.h>
#include "BinaryWriter.h"
#include "BinaryReader.h"

#include "..\\..\\..\\Library\LZ4\\lz4.h"

//�o�C�i���f�[�^�����k�A�𓀂���֐�

extern void BinaryCompress(const char* src, size_t srcSize, std::vector<char>& dst);
extern void BinaryCompressZSD(const char* src, size_t srcSize, std::vector<char>& dst, int compressionLevel);

extern void BinaryDecompress(std::vector<char>& dst, size_t originalSize, const char* src, size_t srcSize);
extern void BinaryDecompressZSD(std::vector<char>& dst, size_t originalSize, const char* src, size_t srcSize);