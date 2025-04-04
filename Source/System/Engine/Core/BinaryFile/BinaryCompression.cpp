#include "BinaryCompression.h"

void BinaryCompress(const char* src, size_t srcSize, std::vector<char>& dst)
{
	/*size_t cBuffSize = ZSTD_compressBound(srcSize);
	dst.resize(cBuffSize);
	size_t cSize = ZSTD_compress(dst.data(), cBuffSize, src, srcSize, compressionLevel);

    if (ZSTD_isError(cSize)) {
        printf("圧縮中にエラーが発生しました。%s\n", ZSTD_getErrorName(cSize));
    }

	dst.resize(cSize);

    ZSTD_CCtx* cctx = ZSTD_createCCtx();
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, compressionLevel);
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, 4);  //4スレッドで並列圧縮

    size_t cBuffSize = ZSTD_compressBound(srcSize);
    dst.resize(cBuffSize);

    size_t cSize = ZSTD_compress2(cctx, dst.data(), cBuffSize, src, srcSize);

    if (ZSTD_isError(cSize)) {
        printf("圧縮中にエラーが発生しました: %s\n", ZSTD_getErrorName(cSize));
    }

    dst.resize(cSize);
    ZSTD_freeCCtx(cctx);*/
    
    // 圧縮後の最大サイズを取得
    int maxCompressedSize = LZ4_compressBound(static_cast<int>(srcSize));
    dst.resize(maxCompressedSize);

    // 圧縮
    int compressedSize = LZ4_compress_default(src, dst.data(), static_cast<int>(srcSize), maxCompressedSize);

    if (compressedSize <= 0) {
        printf("圧縮中にエラーが発生しました。\n");
        return;
    }

    // 必要なサイズにリサイズ
    dst.resize(compressedSize);
}

void BinaryCompressZSD(const char* src, size_t srcSize, std::vector<char>& dst, int compressionLevel)
{
    ZSTD_CCtx* cctx = ZSTD_createCCtx();
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, compressionLevel);
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, 4);  //4スレッドで並列圧縮

    size_t cBuffSize = ZSTD_compressBound(srcSize);
    dst.resize(cBuffSize);

    size_t cSize = ZSTD_compress2(cctx, dst.data(), cBuffSize, src, srcSize);

    if (ZSTD_isError(cSize)) {
        printf("圧縮中にエラーが発生しました: %s\n", ZSTD_getErrorName(cSize));
    }

    dst.resize(cSize);
    ZSTD_freeCCtx(cctx);
}

void BinaryDecompress(std::vector<char>& dst, size_t originalSize, const char* src, size_t srcSize)
{
    /*
    //解凍バッファのサイズを元のサイズに設定
    dst.resize(originalSize);

    //解凍
    size_t const dSize = ZSTD_decompress(dst.data(), originalSize, src, srcSize);

    if (ZSTD_isError(dSize)) {
        printf("解凍中にエラーが発生しました。%s\n", ZSTD_getErrorName(dSize));
    }*/

    dst.resize(originalSize);

    // 解凍
    int decompressedSize = LZ4_decompress_safe(src, dst.data(), static_cast<int>(srcSize), static_cast<int>(originalSize));

    if (decompressedSize < 0) {
        printf("解凍中にエラーが発生しました。\n");
    }
}

void BinaryDecompressZSD(std::vector<char>& dst, size_t originalSize, const char* src, size_t srcSize)
{
    //解凍バッファのサイズを元のサイズに設定
    dst.resize(originalSize);

    //解凍
    size_t const dSize = ZSTD_decompress(dst.data(), originalSize, src, srcSize);

    if (ZSTD_isError(dSize)) {
        printf("解凍中にエラーが発生しました。%s\n", ZSTD_getErrorName(dSize));
    }
}
