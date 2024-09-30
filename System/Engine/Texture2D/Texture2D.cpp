#include "Texture2D.h"

#include "DirectXTex.h"

using namespace DirectX;

// コンストラクタ
Texture2D::Texture2D()
    : m_Count(0)
    , m_State(STATE_INIT)
    , m_RowPitch(0)
    , m_pTexture(nullptr)
    , m_pTextureUpload(nullptr)
{
}

// デストラクタ
Texture2D::~Texture2D()
{
    SAFE_RELEASE(m_pTextureUpload);
    SAFE_RELEASE(m_pTexture);
}

// テクスチャーの作成
bool Texture2D::CreateTexture(ID3D12Device* pDevice, const wchar_t* pFile)
{
    TexMetadata metadata = {};
    ScratchImage scratchImg = {};

    // ファイルの読み込み
    HRESULT result = LoadFromWICFile(
        pFile,
        WIC_FLAGS_NONE,        // オプションなし
        &metadata,
        scratchImg
    );
    if (FAILED(result)) {
        printf("テクスチャの読み込みに失敗しました。");
        return false;
    }

    const Image* pImage = scratchImg.GetImage(0, 0, 0);

    // 画像情報を保持
    m_RowPitch = pImage->rowPitch;

    // リソース作成
    CreateResource(pDevice, pImage, metadata);

    // コピーへ移行
    m_State = STATE_COPY;

    return true;
}

void Texture2D::CreateResource(ID3D12Device* pDevice, const Image* pImage, TexMetadata& metadata)
{
    // ヒープのプロパティ
    D3D12_HEAP_PROPERTIES prop{};
    // ヒープの種類
    prop.Type = D3D12_HEAP_TYPE_UPLOAD;     // cpuは書き込み、gpuは読み取り
    // CPUページプロパティ
    prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN; // プロパティ不明
    // メモリプール
    prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;  // プール不明
    // マルチアダプター関連
    prop.CreationNodeMask = 1;
    prop.VisibleNodeMask = 1;

    // リソースの設定
    D3D12_RESOURCE_DESC desc{};
    // リソースのディメンション
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    // 配置の指定
    desc.Alignment = 0;
    // リソースの幅、高さ（今回はテクスチャサイズ）
    desc.Width = pImage->slicePitch;
    desc.Height = 1;
    // リソース深さ
    desc.DepthOrArraySize = 1;
    // MIPレベル
    desc.MipLevels = 1;
    // リソースデータの形式
    desc.Format = DXGI_FORMAT_UNKNOWN;  // なし
    // マルチサンプリングの設定
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    // テクスチャのレイアウト
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;   // データを連続して配置する
    // オプション
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;  // なし

    // データアップロード用のリソースを作成
    HRESULT result = pDevice->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_pTextureUpload)
    );
    if (FAILED(result)) {
        printf("リソースの作成に失敗しました。");
        return;
    }

    UINT8* pDataBegin = nullptr;
    // リソースデータのポインターを取得
    result = m_pTextureUpload->Map(
        0,          // インデックス番号
        nullptr,    // リソース全体
        reinterpret_cast<void**>(&pDataBegin)
    );
    if (FAILED(result)) {
        printf("リソースデータの取得に失敗しました。");
        return;
    }
    // バッファーに情報をコピー
    memcpy(pDataBegin, pImage->pixels, pImage->slicePitch);
    // 取得したポインターを無効にする
    m_pTextureUpload->Unmap(0, nullptr);

    // テクスチャリソースの作成

    // ヒープの設定
    prop.Type = D3D12_HEAP_TYPE_DEFAULT;

    // リソースの設定
    desc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
    desc.Width = static_cast<UINT64>(metadata.width);
    desc.Height = static_cast<UINT>(metadata.height);
    desc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);
    desc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
    desc.Format = metadata.format;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    // テクスチャのリソースを作成
    result = pDevice->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_pTexture)
    );
    if (FAILED(result)) {
        printf("テクスチャリソースの作成に失敗しました。");
        return;
    }
}

// テクスチャ情報をコピー
void Texture2D::CopyTexture(ID3D12GraphicsCommandList* pCommandList)
{
    if (pCommandList == nullptr || m_pTexture == nullptr) {
        return;
    }

    D3D12_RESOURCE_DESC desc = m_pTexture->GetDesc();

    // テクスチャのコピー情報
    D3D12_TEXTURE_COPY_LOCATION dest{};
    // コピー先
    dest.pResource = m_pTexture;
    // 種類
    dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    // サブリソースのインデックス
    dest.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION src{};
    // コピー元
    src.pResource = m_pTextureUpload;
    // 種類
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    // オフセット
    src.PlacedFootprint.Offset = 0;
    // リソースの情報
    src.PlacedFootprint.Footprint.Format = desc.Format;
    src.PlacedFootprint.Footprint.Width = static_cast<UINT>(desc.Width);
    src.PlacedFootprint.Footprint.Height = static_cast<UINT>(desc.Height);
    src.PlacedFootprint.Footprint.Depth = static_cast<UINT>(desc.DepthOrArraySize);
    src.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(m_RowPitch);

    // テクスチャのコピー
    pCommandList->CopyTextureRegion(
        &dest,
        0,      // 左上のx座標
        0,      // 左上のy座標
        0,      // 左上のz座標
        &src,
        nullptr // 3Dボックスの設定
    );

    // リソースの状態を遷移させる
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = m_pTexture;
    // コピー先
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    // ピクセルシェーダー
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
            // コピーが終わったら消してもいい
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

// シェーダーリソースビューの作成
void Texture2D::CreateSRV(ID3D12Device* pDevice, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    if (m_pTexture == nullptr)  return;

    D3D12_RESOURCE_DESC desc = m_pTexture->GetDesc();

    // シェーダーリソースビューの設定
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

    // デフォルト
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    // フォーマット
    srvDesc.Format = desc.Format;
    // 2Dテクスチャ
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    // ミップレベル
    srvDesc.Texture2D.MipLevels = static_cast<UINT>(desc.MipLevels);

    // シェーダーリソースビューの作成
    pDevice->CreateShaderResourceView(m_pTexture, &srvDesc, handle);
}
