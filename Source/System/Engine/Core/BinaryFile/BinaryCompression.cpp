#include "BinaryCompression.h"

void BinaryCompress(const char* src, size_t srcSize, std::vector<char>& dst, int compressionLevel)
{
	size_t cBuffSize = ZSTD_compressBound(srcSize);
	dst.resize(cBuffSize);
	size_t cSize = ZSTD_compress(dst.data(), cBuffSize, src, srcSize, compressionLevel);

    if (ZSTD_isError(cSize)) {
        printf("���k���ɃG���[���������܂����B%s\n", ZSTD_getErrorName(cSize));
    }

	dst.resize(cSize);
}

void BinaryDecompress(std::vector<char>& dst, size_t originalSize, const char* src, size_t srcSize)
{
    //�𓀃o�b�t�@�̃T�C�Y�����̃T�C�Y�ɐݒ�
    dst.resize(originalSize);

    //��
    size_t const dSize = ZSTD_decompress(dst.data(), originalSize, src, srcSize);

    if (ZSTD_isError(dSize)) {
        printf("�𓀒��ɃG���[���������܂����B%s\n", ZSTD_getErrorName(dSize));
    }
}
