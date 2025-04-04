#include "FontManager.h"

FontManager::FontManager()
    : m_pDevice(nullptr)
    , m_pCommandList(nullptr)
    , m_library(nullptr)
{
}

void FontManager::Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList)
{
    //FreeType���C�u�����̏�����
    if (FT_Init_FreeType(&m_library)) {
        printf("FreeType�̏������Ɏ��s���܂����B\n");
        return;
    }

    m_pDevice = pDevice;
    m_pCommandList = pCommandList;
}

FontRef FontManager::CreateFont(std::string strFilePath, UINT fontSize)
{
    std::string saveName = strFilePath + ":" + std::to_string(fontSize);
    if (m_registerdFonts.find(saveName) != m_registerdFonts.end()) {
        return m_registerdFonts[saveName];
    }

    FontRef fontRef = std::make_shared<Font>(m_pDevice, m_pCommandList, strFilePath, fontSize);

    //�t�H���g�t�@�C���̓ǂݍ���
    if (FT_New_Face(m_library, strFilePath.c_str(), 0, &fontRef->m_fontFace)) {
        printf("�t�H���g�t�@�C���̓ǂݍ��݂Ɏ��s���܂����B\n");
        return nullptr;
    }

    //�t�H���g�T�C�Y�̐ݒ�i��: 48px�̃t�H���g�T�C�Y�j
    if (FT_Set_Pixel_Sizes(fontRef->m_fontFace, fontSize, fontSize)) {
        printf("�t�H���g�T�C�Y�̐ݒ�Ɏ��s���܂����B\n");
        return nullptr;
    }

    m_registerdFonts.emplace(saveName, fontRef);

    return fontRef;
}
