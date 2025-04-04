#include "Font.h"

Font::Font(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const std::string strFilePath, UINT fontSize)
    : m_pDevice(pDevice)
    , m_pCommandList(pCommandList)
    , m_fontFile(strFilePath)
    , m_atlasWidth(ATLAS_DEFAULT_SIZE)
    , m_atlasHeight(ATLAS_DEFAULT_SIZE)
    , m_fontSize(fontSize)
    , m_fontFace(nullptr)
    , m_mappedData(nullptr)
    , m_currentX(0)
    , m_currentY(0)
    , m_rowHeight(0)
{
    //アトラスバッファを作成
    CreateAtlasBuffer();
    //アップロードバッファを作成
    CreateUploadBuffer();
}

Font::~Font()
{
    m_uploadBuffer->Unmap(0, nullptr);
}

bool Font::AddCharacter(wchar_t& character)
{
    //すでに追加されている文字の場合は何もしない
    if (m_addedCharacters.find(character) != m_addedCharacters.end()) {
        return false;
    }

    //FreeTypeを使用して文字を読み込む
    if (FT_Load_Char(m_fontFace, character, FT_LOAD_RENDER)) {
        printf("文字 '%c'の読み込みに失敗しました。\n", character);
        return false;
    }

    //文字のビットマップ情報
    FT_Bitmap& bitmap = m_fontFace->glyph->bitmap;
    unsigned int width = bitmap.width;
    unsigned int height = bitmap.rows;

    //アトラスに文字を追加できるかチェック
    if (m_currentX + width > m_atlasWidth) {
        //横に収まりきれない場合は新しい行に
        m_currentX = 0;
        m_currentY += m_rowHeight;
        m_rowHeight = 0;
    }

    if (m_currentY + height > m_atlasHeight) {
        printf("アトラスのサイズが足りません。\n");
        return false;
    }

    //グリフの位置を計算して追加
    GlyphInfo glyphInfo{};
    glyphInfo.x = m_currentX;
    glyphInfo.y = m_currentY;
    glyphInfo.width = width;
    glyphInfo.height = height;
    glyphInfo.left = m_fontFace->glyph->bitmap_left;
    glyphInfo.top = m_fontFace->glyph->bitmap_top;
    glyphInfo.uvMinX = static_cast<float>(m_currentX) / static_cast<float>(m_atlasWidth);
    glyphInfo.uvMinY = static_cast<float>(m_currentY) / static_cast<float>(m_atlasHeight);
    glyphInfo.uvMaxX = static_cast<float>(m_currentX + width) / static_cast<float>(m_atlasWidth);
    glyphInfo.uvMaxY = static_cast<float>(m_currentY + height) / static_cast<float>(m_atlasHeight);

    m_glyphs.emplace(character, glyphInfo);

    //文字を追加済みとしてセットに記録
    m_addedCharacters.insert(character);

    //次の位置に移動
    m_currentX += width + 1;
    m_rowHeight = max(m_rowHeight, height);

    //サイズチェック
    UINT rowPitch = (width + 255) & ~255; //256バイトアライメント
    UINT slicePitch = rowPitch * height;

    if (slicePitch > m_uploadBuffer->GetDesc().Width) {
        printf("アップロードバッファのサイズが不足しています。\n");
        return false;
    }

    //アップロードバッファにデータを書き込み
    uint8_t* startAddress = reinterpret_cast<uint8_t*>(m_mappedData) + glyphInfo.y * m_atlasWidth + glyphInfo.x;

    //各行ごとにコピー
    for (unsigned int y = 0; y < height; ++y) {
        uint8_t* dst = startAddress + y * m_atlasWidth;     //アトラス側の位置
        uint8_t* src = bitmap.buffer + y * bitmap.pitch;    //FreeType側の位置
        memcpy(dst, src, width);
    }

    return true;
}

void Font::UpdateAtlas()
{
    //アトラスデータのコピーは解像度に依存して時間がかかるため非同期で行う
    std::thread([=]() {
        g_resourceCopy->BeginCopyResource();

        //リソースステートをCOPY_DESTに変更
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_atlasResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
        g_resourceCopy->GetCommandList()->ResourceBarrier(1, &barrier);

        //サブリソースデータの設定
        D3D12_SUBRESOURCE_DATA subresourceData{};
        subresourceData.pData = m_mappedData;
        subresourceData.RowPitch = m_atlasWidth;
        subresourceData.SlicePitch = static_cast<UINT64>(m_atlasWidth * m_atlasHeight);

        //転送
        UpdateSubresources(g_resourceCopy->GetCommandList(), m_atlasResource.Get(), m_uploadBuffer.Get(), 0, 0, 1, &subresourceData);

        //リソースステートを戻す
        CD3DX12_RESOURCE_BARRIER barrierBack = CD3DX12_RESOURCE_BARRIER::Transition(m_atlasResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        g_resourceCopy->GetCommandList()->ResourceBarrier(1, &barrierBack);

        //処理開始
        g_resourceCopy->EndCopyResource();

        }).detach();

    //HRESULT hr = m_atlasResource->WriteToSubresource(0, nullptr, m_mappedData, m_atlasWidth, m_atlasWidth * m_atlasHeight);
}

void Font::Release() const
{
    if (m_fontFace) {
        FT_Done_Face(m_fontFace);
    }
}

void Font::CreateAtlasBuffer()
{
    CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UNORM, m_atlasWidth, m_atlasHeight, 1, 1);
    CD3DX12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    HRESULT result = m_pDevice->CreateCommittedResource(&texHeapProp, D3D12_HEAP_FLAG_NONE, &resDesc, state, nullptr, IID_PPV_ARGS(&m_atlasResource));

    if (FAILED(result)) {
        printf("GetDefaultResource : エラーコード = %1x\n", result);
        return;
    }

    //初期値を挿入
    std::vector<BYTE> data(m_atlasWidth * m_atlasHeight);
    std::fill(data.begin(), data.end(), (BYTE)255);

    HRESULT hr = m_atlasResource->WriteToSubresource(0, nullptr, data.data(), m_atlasWidth * sizeof(BYTE), static_cast<BYTE>(data.size() * sizeof(BYTE)));
    if (FAILED(hr)) {
        printf("アトラスリソースの初期化に失敗しました。\n");
        return;
    }
}

void Font::CreateUploadBuffer()
{
    D3D12_HEAP_PROPERTIES uploadHeapProperties = {};
    uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(m_atlasWidth * m_atlasHeight));

    HRESULT hr = m_pDevice->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_uploadBuffer)
    );

    if (FAILED(hr)) {
        printf("アップロード用のリソース作成に失敗しました。\n");
        return;
    }

    m_uploadBuffer->Map(0, nullptr, &m_mappedData);
}
