#include "BinaryCompression.h"

void BinaryCompress(const char* src, size_t srcSize, std::vector<char>& dst, int compressionLevel)
{
	size_t cBuffSize = ZSTD_compressBound(srcSize);
	dst.resize(cBuffSize);
	size_t cSize = ZSTD_compress(dst.data(), cBuffSize, src, srcSize, compressionLevel);

    if (ZSTD_isError(cSize)) {
        printf("圧縮中にエラーが発生しました。%s\n", ZSTD_getErrorName(cSize));
    }

	dst.resize(cSize);
}

void BinaryDecompress(std::vector<char>& dst, size_t originalSize, const char* src, size_t srcSize)
{
    //解凍バッファのサイズを元のサイズに設定
    dst.resize(originalSize);

    //解凍
    size_t const dSize = ZSTD_decompress(dst.data(), originalSize, src, srcSize);

    if (ZSTD_isError(dSize)) {
        printf("解凍中にエラーが発生しました。%s\n", ZSTD_getErrorName(dSize));
    }
}
