#include "ResourceCopy.h"

ResourceCopy* g_resourceCopy = nullptr;

ResourceCopy::ResourceCopy()
    : m_pDevice(nullptr)
    , m_fenceValue(0)
    , m_bCanExecute(false)
{
    m_eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_eventHandle) {
        printf("イベントの作成に失敗しました。\n");
    }
}

ID3D12Resource* ResourceCopy::CreateUploadBuffer(UINT64 bufferSize)
{
    //アップロード用の一時バッファを作成
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    ID3D12Resource* pBuffer;

    HRESULT hr = m_pDevice->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pBuffer)
    );
    if (FAILED(hr)) {
        printf("UploadBufferの作成に失敗しました。\n");
        return nullptr;
    }

    return pBuffer;
}

void ResourceCopy::Initialize(ID3D12Device* pDevice)
{
    m_pDevice = pDevice;

    //コマンドアロケータとコマンドリストの作成
    HRESULT hr = pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_allocator));
    if (FAILED(hr)) {
        printf("コマンドアロケータの作成に失敗しました。\n");
    }

    hr = pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_allocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList));
    if (FAILED(hr)) {
        printf("コマンドリストの作成に失敗しました。\n");
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    hr = pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
    if (FAILED(hr)) {
        printf("コマンドキューの作成に失敗しました。\n");
    }

    hr = pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
    if (FAILED(hr)) {
        printf("フェンスの作成に失敗しました。\n");
    }

    m_commandList->Close();

    m_bCanExecute = true;
}

void ResourceCopy::BeginCopyResource()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    //ほかのスレッドがコピー中の場合は待機
    m_condition.wait(lock, [this]() { return m_bCanExecute; });

    m_bCanExecute = false;

    //コマンドを初期化してためる準備をする
    HRESULT hr = m_allocator->Reset();
    if (FAILED(hr)) {
        printf("コマンドアロケータのリセットに失敗しました。\n");
    }

    hr = m_commandList->Reset(m_allocator.Get(), nullptr);
    if (FAILED(hr)) {
        printf("コマンドリストのリセットに失敗しました。\n");
    }
}

void ResourceCopy::EndCopyResource()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    HRESULT hr = m_commandList->Close();
    if (FAILED(hr)) {
        printf("コマンドリストのクローズに失敗しました。\n");
    }

    ID3D12CommandList* commandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(1, commandLists);

    //フェンスをシグナル
    const UINT64 currentFenceValue = ++m_fenceValue;
    hr = m_commandQueue->Signal(m_fence.Get(), currentFenceValue);
    if (FAILED(hr)) {
        printf("フェンスシグナルに失敗しました。");
    }

    //完了待機
    hr = m_fence->SetEventOnCompletion(currentFenceValue, m_eventHandle);
    if (FAILED(hr)) {
        printf("フェンスイベント設定に失敗しました。");
    }

    WaitForSingleObject(m_eventHandle, INFINITE);

    //完了
    m_bCanExecute = true;
    m_condition.notify_one();
}
