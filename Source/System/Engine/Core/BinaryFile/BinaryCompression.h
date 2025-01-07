#pragma once
#include <zstd.h>
#include "BinaryWriter.h"
#include "BinaryReader.h"

//�o�C�i���f�[�^�����k�A�𓀂���֐�

extern void BinaryCompress(const char* src, size_t srcSize, std::vector<char>& dst, int compressionLevel);

extern void BinaryDecompress(std::vector<char>& dst, size_t originalSize, const char* src, size_t srcSize);