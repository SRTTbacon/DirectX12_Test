#include "BinaryCompression.h"

void BinaryCompress(const char* src, size_t srcSize, std::vector<char>& dst)
{
	/*size_t cBuffSize = ZSTD_compressBound(srcSize);
	dst.resize(cBuffSize);
	size_t cSize = ZSTD_compress(dst.data(), cBuffSize, src, srcSize, compressionLevel);

    if (ZSTD_isError(cSize)) {
        printf("���k���ɃG���[���������܂����B%s\n", ZSTD_getErrorName(cSize));
    }

	dst.resize(cSize);

    ZSTD_CCtx* cctx = ZSTD_createCCtx();
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, compressionLevel);
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, 4);  //4�X���b�h�ŕ��񈳏k

    size_t cBuffSize = ZSTD_compressBound(srcSize);
    dst.resize(cBuffSize);

    size_t cSize = ZSTD_compress2(cctx, dst.data(), cBuffSize, src, srcSize);

    if (ZSTD_isError(cSize)) {
        printf("���k���ɃG���[���������܂���: %s\n", ZSTD_getErrorName(cSize));
    }

    dst.resize(cSize);
    ZSTD_freeCCtx(cctx);*/
    
    // ���k��̍ő�T�C�Y���擾
    int maxCompressedSize = LZ4_compressBound(static_cast<int>(srcSize));
    dst.resize(maxCompressedSize);

    // ���k
    int compressedSize = LZ4_compress_default(src, dst.data(), static_cast<int>(srcSize), maxCompressedSize);

    if (compressedSize <= 0) {
        printf("���k���ɃG���[���������܂����B\n");
        return;
    }

    // �K�v�ȃT�C�Y�Ƀ��T�C�Y
    dst.resize(compressedSize);
}

void BinaryCompressZSD(const char* src, size_t srcSize, std::vector<char>& dst, int compressionLevel)
{
    ZSTD_CCtx* cctx = ZSTD_createCCtx();
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, compressionLevel);
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, 4);  //4�X���b�h�ŕ��񈳏k

    size_t cBuffSize = ZSTD_compressBound(srcSize);
    dst.resize(cBuffSize);

    size_t cSize = ZSTD_compress2(cctx, dst.data(), cBuffSize, src, srcSize);

    if (ZSTD_isError(cSize)) {
        printf("���k���ɃG���[���������܂���: %s\n", ZSTD_getErrorName(cSize));
    }

    dst.resize(cSize);
    ZSTD_freeCCtx(cctx);
}

void BinaryDecompress(std::vector<char>& dst, size_t originalSize, const char* src, size_t srcSize)
{
    /*
    //�𓀃o�b�t�@�̃T�C�Y�����̃T�C�Y�ɐݒ�
    dst.resize(originalSize);

    //��
    size_t const dSize = ZSTD_decompress(dst.data(), originalSize, src, srcSize);

    if (ZSTD_isError(dSize)) {
        printf("�𓀒��ɃG���[���������܂����B%s\n", ZSTD_getErrorName(dSize));
    }*/

    dst.resize(originalSize);

    // ��
    int decompressedSize = LZ4_decompress_safe(src, dst.data(), static_cast<int>(srcSize), static_cast<int>(originalSize));

    if (decompressedSize < 0) {
        printf("�𓀒��ɃG���[���������܂����B\n");
    }
}

void BinaryDecompressZSD(std::vector<char>& dst, size_t originalSize, const char* src, size_t srcSize)
{
    //�𓀃o�b�t�@�̃T�C�Y�����̃T�C�Y�ɐݒ�
    dst.resize(originalSize);

    //��
    size_t const dSize = ZSTD_decompress(dst.data(), originalSize, src, srcSize);

    if (ZSTD_isError(dSize)) {
        printf("�𓀒��ɃG���[���������܂����B%s\n", ZSTD_getErrorName(dSize));
    }
}
