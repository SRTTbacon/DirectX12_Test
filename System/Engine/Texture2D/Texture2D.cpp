#include "Texture2D.h"

#include "DirectXTex.h"

using namespace DirectX;

// �R���X�g���N�^
Texture2D::Texture2D()
    : m_Count(0)
    , m_State(STATE_INIT)
    , m_RowPitch(0)
    , m_pTexture(nullptr)
    , m_pTextureUpload(nullptr)
{
}

// �f�X�g���N�^
Texture2D::~Texture2D()
{
    SAFE_RELEASE(m_pTextureUpload);
    SAFE_RELEASE(m_pTexture);
}

// �e�N�X�`���[�̍쐬
bool Texture2D::CreateTexture(ID3D12Device* pDevice, const wchar_t* pFile)
{
    TexMetadata metadata = {};
    ScratchImage scratchImg = {};

    // �t�@�C���̓ǂݍ���
    HRESULT result = LoadFromWICFile(
        pFile,
        WIC_FLAGS_NONE,        // �I�v�V�����Ȃ�
        &metadata,
        scratchImg
    );
    if (FAILED(result)) {
        printf("�e�N�X�`���̓ǂݍ��݂Ɏ��s���܂����B");
        return false;
    }

    const Image* pImage = scratchImg.GetImage(0, 0, 0);

    // �摜����ێ�
    m_RowPitch = pImage->rowPitch;

    // ���\�[�X�쐬
    CreateResource(pDevice, pImage, metadata);

    // �R�s�[�ֈڍs
    m_State = STATE_COPY;

    return true;
}

void Texture2D::CreateResource(ID3D12Device* pDevice, const Image* pImage, TexMetadata& metadata)
{
    // �q�[�v�̃v���p�e�B
    D3D12_HEAP_PROPERTIES prop{};
    // �q�[�v�̎��
    prop.Type = D3D12_HEAP_TYPE_UPLOAD;     // cpu�͏������݁Agpu�͓ǂݎ��
    // CPU�y�[�W�v���p�e�B
    prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN; // �v���p�e�B�s��
    // �������v�[��
    prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;  // �v�[���s��
    // �}���`�A�_�v�^�[�֘A
    prop.CreationNodeMask = 1;
    prop.VisibleNodeMask = 1;

    // ���\�[�X�̐ݒ�
    D3D12_RESOURCE_DESC desc{};
    // ���\�[�X�̃f�B�����V����
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    // �z�u�̎w��
    desc.Alignment = 0;
    // ���\�[�X�̕��A�����i����̓e�N�X�`���T�C�Y�j
    desc.Width = pImage->slicePitch;
    desc.Height = 1;
    // ���\�[�X�[��
    desc.DepthOrArraySize = 1;
    // MIP���x��
    desc.MipLevels = 1;
    // ���\�[�X�f�[�^�̌`��
    desc.Format = DXGI_FORMAT_UNKNOWN;  // �Ȃ�
    // �}���`�T���v�����O�̐ݒ�
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    // �e�N�X�`���̃��C�A�E�g
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;   // �f�[�^��A�����Ĕz�u����
    // �I�v�V����
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;  // �Ȃ�

    // �f�[�^�A�b�v���[�h�p�̃��\�[�X���쐬
    HRESULT result = pDevice->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_pTextureUpload)
    );
    if (FAILED(result)) {
        printf("���\�[�X�̍쐬�Ɏ��s���܂����B");
        return;
    }

    UINT8* pDataBegin = nullptr;
    // ���\�[�X�f�[�^�̃|�C���^�[���擾
    result = m_pTextureUpload->Map(
        0,          // �C���f�b�N�X�ԍ�
        nullptr,    // ���\�[�X�S��
        reinterpret_cast<void**>(&pDataBegin)
    );
    if (FAILED(result)) {
        printf("���\�[�X�f�[�^�̎擾�Ɏ��s���܂����B");
        return;
    }
    // �o�b�t�@�[�ɏ����R�s�[
    memcpy(pDataBegin, pImage->pixels, pImage->slicePitch);
    // �擾�����|�C���^�[�𖳌��ɂ���
    m_pTextureUpload->Unmap(0, nullptr);

    // �e�N�X�`�����\�[�X�̍쐬

    // �q�[�v�̐ݒ�
    prop.Type = D3D12_HEAP_TYPE_DEFAULT;

    // ���\�[�X�̐ݒ�
    desc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
    desc.Width = static_cast<UINT64>(metadata.width);
    desc.Height = static_cast<UINT>(metadata.height);
    desc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);
    desc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
    desc.Format = metadata.format;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    // �e�N�X�`���̃��\�[�X���쐬
    result = pDevice->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_pTexture)
    );
    if (FAILED(result)) {
        printf("�e�N�X�`�����\�[�X�̍쐬�Ɏ��s���܂����B");
        return;
    }
}

// �e�N�X�`�������R�s�[
void Texture2D::CopyTexture(ID3D12GraphicsCommandList* pCommandList)
{
    if (pCommandList == nullptr || m_pTexture == nullptr) {
        return;
    }

    D3D12_RESOURCE_DESC desc = m_pTexture->GetDesc();

    // �e�N�X�`���̃R�s�[���
    D3D12_TEXTURE_COPY_LOCATION dest{};
    // �R�s�[��
    dest.pResource = m_pTexture;
    // ���
    dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    // �T�u���\�[�X�̃C���f�b�N�X
    dest.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION src{};
    // �R�s�[��
    src.pResource = m_pTextureUpload;
    // ���
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    // �I�t�Z�b�g
    src.PlacedFootprint.Offset = 0;
    // ���\�[�X�̏��
    src.PlacedFootprint.Footprint.Format = desc.Format;
    src.PlacedFootprint.Footprint.Width = static_cast<UINT>(desc.Width);
    src.PlacedFootprint.Footprint.Height = static_cast<UINT>(desc.Height);
    src.PlacedFootprint.Footprint.Depth = static_cast<UINT>(desc.DepthOrArraySize);
    src.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(m_RowPitch);

    // �e�N�X�`���̃R�s�[
    pCommandList->CopyTextureRegion(
        &dest,
        0,      // �����x���W
        0,      // �����y���W
        0,      // �����z���W
        &src,
        nullptr // 3D�{�b�N�X�̐ݒ�
    );

    // ���\�[�X�̏�Ԃ�J�ڂ�����
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = m_pTexture;
    // �R�s�[��
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    // �s�N�Z���V�F�[�_�[
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    pCommandList->ResourceBarrier(1, &barrier);

    m_Count = 0;
    m_State = STATE_COPY_WAIT;
}

void Texture2D::Update(ID3D12GraphicsCommandList* pCommandList)
{
    switch (m_State)
    {
    case STATE_COPY:
        CopyTexture(pCommandList);
        break;

    case STATE_COPY_WAIT:
        if (++m_Count > 60)
        {
            // �R�s�[���I�����������Ă�����
            SAFE_RELEASE(m_pTextureUpload);

            m_State = STATE_IDLE;
        }
        break;

    case STATE_IDLE:
        break;

    default:
        ;
    }
}

// �V�F�[�_�[���\�[�X�r���[�̍쐬
void Texture2D::CreateSRV(ID3D12Device* pDevice, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    if (m_pTexture == nullptr)  return;

    D3D12_RESOURCE_DESC desc = m_pTexture->GetDesc();

    // �V�F�[�_�[���\�[�X�r���[�̐ݒ�
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

    // �f�t�H���g
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    // �t�H�[�}�b�g
    srvDesc.Format = desc.Format;
    // 2D�e�N�X�`��
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    // �~�b�v���x��
    srvDesc.Texture2D.MipLevels = static_cast<UINT>(desc.MipLevels);

    // �V�F�[�_�[���\�[�X�r���[�̍쐬
    pDevice->CreateShaderResourceView(m_pTexture, &srvDesc, handle);
}
