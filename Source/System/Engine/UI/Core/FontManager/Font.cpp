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
    //�A�g���X�o�b�t�@���쐬
    CreateAtlasBuffer();
    //�A�b�v���[�h�o�b�t�@���쐬
    CreateUploadBuffer();
}

Font::~Font()
{
    m_uploadBuffer->Unmap(0, nullptr);
}

bool Font::AddCharacter(wchar_t& character)
{
    //���łɒǉ�����Ă��镶���̏ꍇ�͉������Ȃ�
    if (m_addedCharacters.find(character) != m_addedCharacters.end()) {
        return false;
    }

    //FreeType���g�p���ĕ�����ǂݍ���
    if (FT_Load_Char(m_fontFace, character, FT_LOAD_RENDER)) {
        printf("���� '%c'�̓ǂݍ��݂Ɏ��s���܂����B\n", character);
        return false;
    }

    //�����̃r�b�g�}�b�v���
    FT_Bitmap& bitmap = m_fontFace->glyph->bitmap;
    unsigned int width = bitmap.width;
    unsigned int height = bitmap.rows;

    //�A�g���X�ɕ�����ǉ��ł��邩�`�F�b�N
    if (m_currentX + width > m_atlasWidth) {
        //���Ɏ��܂肫��Ȃ��ꍇ�͐V�����s��
        m_currentX = 0;
        m_currentY += m_rowHeight;
        m_rowHeight = 0;
    }

    if (m_currentY + height > m_atlasHeight) {
        printf("�A�g���X�̃T�C�Y������܂���B\n");
        return false;
    }

    //�O���t�̈ʒu���v�Z���Ēǉ�
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

    //������ǉ��ς݂Ƃ��ăZ�b�g�ɋL�^
    m_addedCharacters.insert(character);

    //���̈ʒu�Ɉړ�
    m_currentX += width + 1;
    m_rowHeight = max(m_rowHeight, height);

    //�T�C�Y�`�F�b�N
    UINT rowPitch = (width + 255) & ~255; //256�o�C�g�A���C�����g
    UINT slicePitch = rowPitch * height;

    if (slicePitch > m_uploadBuffer->GetDesc().Width) {
        printf("�A�b�v���[�h�o�b�t�@�̃T�C�Y���s�����Ă��܂��B\n");
        return false;
    }

    //�A�b�v���[�h�o�b�t�@�Ƀf�[�^����������
    uint8_t* startAddress = reinterpret_cast<uint8_t*>(m_mappedData) + glyphInfo.y * m_atlasWidth + glyphInfo.x;

    //�e�s���ƂɃR�s�[
    for (unsigned int y = 0; y < height; ++y) {
        uint8_t* dst = startAddress + y * m_atlasWidth;     //�A�g���X���̈ʒu
        uint8_t* src = bitmap.buffer + y * bitmap.pitch;    //FreeType���̈ʒu
        memcpy(dst, src, width);
    }

    return true;
}

void Font::UpdateAtlas()
{
    //�A�g���X�f�[�^�̃R�s�[�͉𑜓x�Ɉˑ����Ď��Ԃ������邽�ߔ񓯊��ōs��
    std::thread([=]() {
        g_resourceCopy->BeginCopyResource();

        //���\�[�X�X�e�[�g��COPY_DEST�ɕύX
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_atlasResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
        g_resourceCopy->GetCommandList()->ResourceBarrier(1, &barrier);

        //�T�u���\�[�X�f�[�^�̐ݒ�
        D3D12_SUBRESOURCE_DATA subresourceData{};
        subresourceData.pData = m_mappedData;
        subresourceData.RowPitch = m_atlasWidth;
        subresourceData.SlicePitch = static_cast<UINT64>(m_atlasWidth * m_atlasHeight);

        //�]��
        UpdateSubresources(g_resourceCopy->GetCommandList(), m_atlasResource.Get(), m_uploadBuffer.Get(), 0, 0, 1, &subresourceData);

        //���\�[�X�X�e�[�g��߂�
        CD3DX12_RESOURCE_BARRIER barrierBack = CD3DX12_RESOURCE_BARRIER::Transition(m_atlasResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        g_resourceCopy->GetCommandList()->ResourceBarrier(1, &barrierBack);

        //�����J�n
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
        printf("GetDefaultResource : �G���[�R�[�h = %1x\n", result);
        return;
    }

    //�����l��}��
    std::vector<BYTE> data(m_atlasWidth * m_atlasHeight);
    std::fill(data.begin(), data.end(), (BYTE)255);

    HRESULT hr = m_atlasResource->WriteToSubresource(0, nullptr, data.data(), m_atlasWidth * sizeof(BYTE), static_cast<BYTE>(data.size() * sizeof(BYTE)));
    if (FAILED(hr)) {
        printf("�A�g���X���\�[�X�̏������Ɏ��s���܂����B\n");
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
        printf("�A�b�v���[�h�p�̃��\�[�X�쐬�Ɏ��s���܂����B\n");
        return;
    }

    m_uploadBuffer->Map(0, nullptr, &m_mappedData);
}
