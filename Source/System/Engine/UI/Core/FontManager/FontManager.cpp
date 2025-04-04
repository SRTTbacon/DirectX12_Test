#include "FontManager.h"

FontManager::FontManager()
    : m_pDevice(nullptr)
    , m_pCommandList(nullptr)
    , m_library(nullptr)
{
}

void FontManager::Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList)
{
    //FreeTypeライブラリの初期化
    if (FT_Init_FreeType(&m_library)) {
        printf("FreeTypeの初期化に失敗しました。\n");
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

    //フォントファイルの読み込み
    if (FT_New_Face(m_library, strFilePath.c_str(), 0, &fontRef->m_fontFace)) {
        printf("フォントファイルの読み込みに失敗しました。\n");
        return nullptr;
    }

    //フォントサイズの設定（例: 48pxのフォントサイズ）
    if (FT_Set_Pixel_Sizes(fontRef->m_fontFace, fontSize, fontSize)) {
        printf("フォントサイズの設定に失敗しました。\n");
        return nullptr;
    }

    m_registerdFonts.emplace(saveName, fontRef);

    return fontRef;
}
