#include "ResourceCopy.h"

ResourceCopy* g_resourceCopy = nullptr;

ResourceCopy::ResourceCopy()
    : m_pDevice(nullptr)
    , m_fenceValue(0)
    , m_bCanExecute(false)
{
    m_eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_eventHandle) {
        printf("�C�x���g�̍쐬�Ɏ��s���܂����B\n");
    }
}

ID3D12Resource* ResourceCopy::CreateUploadBuffer(UINT64 bufferSize)
{
    //�A�b�v���[�h�p�̈ꎞ�o�b�t�@���쐬
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    ID3D12Resource* pBuffer;

    HRESULT hr = m_pDevice->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pBuffer)
    );
    if (FAILED(hr)) {
        printf("UploadBuffer�̍쐬�Ɏ��s���܂����B\n");
        return nullptr;
    }

    return pBuffer;
}

void ResourceCopy::Initialize(ID3D12Device* pDevice)
{
    m_pDevice = pDevice;

    //�R�}���h�A���P�[�^�ƃR�}���h���X�g�̍쐬
    HRESULT hr = pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_allocator));
    if (FAILED(hr)) {
        printf("�R�}���h�A���P�[�^�̍쐬�Ɏ��s���܂����B\n");
    }

    hr = pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_allocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList));
    if (FAILED(hr)) {
        printf("�R�}���h���X�g�̍쐬�Ɏ��s���܂����B\n");
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    hr = pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
    if (FAILED(hr)) {
        printf("�R�}���h�L���[�̍쐬�Ɏ��s���܂����B\n");
    }

    hr = pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
    if (FAILED(hr)) {
        printf("�t�F���X�̍쐬�Ɏ��s���܂����B\n");
    }

    m_commandList->Close();

    m_bCanExecute = true;
}

void ResourceCopy::BeginCopyResource()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    //�ق��̃X���b�h���R�s�[���̏ꍇ�͑ҋ@
    m_condition.wait(lock, [this]() { return m_bCanExecute; });

    m_bCanExecute = false;

    //�R�}���h�����������Ă��߂鏀��������
    HRESULT hr = m_allocator->Reset();
    if (FAILED(hr)) {
        printf("�R�}���h�A���P�[�^�̃��Z�b�g�Ɏ��s���܂����B\n");
    }

    hr = m_commandList->Reset(m_allocator.Get(), nullptr);
    if (FAILED(hr)) {
        printf("�R�}���h���X�g�̃��Z�b�g�Ɏ��s���܂����B\n");
    }
}

void ResourceCopy::EndCopyResource()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    HRESULT hr = m_commandList->Close();
    if (FAILED(hr)) {
        printf("�R�}���h���X�g�̃N���[�Y�Ɏ��s���܂����B\n");
    }

    ID3D12CommandList* commandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(1, commandLists);

    //�t�F���X���V�O�i��
    const UINT64 currentFenceValue = ++m_fenceValue;
    hr = m_commandQueue->Signal(m_fence.Get(), currentFenceValue);
    if (FAILED(hr)) {
        printf("�t�F���X�V�O�i���Ɏ��s���܂����B");
    }

    //�����ҋ@
    hr = m_fence->SetEventOnCompletion(currentFenceValue, m_eventHandle);
    if (FAILED(hr)) {
        printf("�t�F���X�C�x���g�ݒ�Ɏ��s���܂����B");
    }

    WaitForSingleObject(m_eventHandle, INFINITE);

    //����
    m_bCanExecute = true;
    m_condition.notify_one();
}
