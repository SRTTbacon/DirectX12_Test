#include "UIManager.h"

using namespace DirectX;

const UINT UIManager::DESCRIPTOR_COUNT = 5000;

UIManager::UIManager()
    : m_pDevice(nullptr)
    , m_pCommandList(nullptr)
    , m_pScreenSize(nullptr)
    , m_vertexBufferView(D3D12_VERTEX_BUFFER_VIEW())
	, m_indexBufferView(D3D12_INDEX_BUFFER_VIEW())
    , m_uiRootSignature(RootSignature())
    , m_uiPipelineState(PipelineState())
    , m_nextDescriptorIndex(0)
{
}

void UIManager::Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, SIZE* pScreenSize)
{
    m_pDevice = pDevice;
    m_pCommandList = pCommandList;
    m_pScreenSize = pScreenSize;

    m_uiRootSignature = RootSignature(pDevice, ShaderKinds::UITextureShader);
    m_uiPipelineState = PipelineState(pDevice, &m_uiRootSignature, D3D12_CULL_MODE_FRONT);

    CreateVertexBuffer();
    CreateIndexBuffer();

    m_descriptorHeap.Initialize(pDevice, DESCRIPTOR_COUNT);

    m_fontManager.Initialize(pDevice, pCommandList);

    m_bCanAccessDescriptorIndexes = std::vector<bool>(DESCRIPTOR_COUNT, true);
}

UITextureRef UIManager::AddUITexture()
{
    UITextureRef uiTexture = std::make_shared<UITexture>(m_pDevice, m_pCommandList, m_pScreenSize, &m_descriptorHeap, GetCanAccessDescriptorIndex());
    uiTexture->Initialize(m_vertexBuffer.Get(), m_indexBuffer.Get(), &m_vertexBufferView, &m_indexBufferView);
    m_elements.push_back(uiTexture);
    return uiTexture;
}

UITextureRef UIManager::AddUITexture(std::string strFilePath)
{
    UITextureRef pUITexture = AddUITexture();
    pUITexture->SetTexture(strFilePath);
    pUITexture->m_size = pUITexture->GetTextureSize();
    return pUITexture;
}

UITextureRef UIManager::AddUITexture(const char* data, size_t size)
{
    UITextureRef pUITexture = AddUITexture();
    pUITexture->SetTexture(data, size);
    pUITexture->m_size = pUITexture->GetTextureSize();
    return pUITexture;
}

UITextureRef UIManager::AddUITexture(DXGI_RGBA color)
{
    UITextureRef pUITexture = AddUITexture();
    pUITexture->SetTexture(color);
    return pUITexture;
}

UITextRef UIManager::AddUIText(std::string strFontPath, UINT fontSize)
{
    FontRef font = m_fontManager.CreateFont(strFontPath, fontSize);
    UITextRef uiText = std::make_shared<UIText>(m_pDevice, m_pCommandList, m_pScreenSize, &m_descriptorHeap, GetCanAccessDescriptorIndex(), font);
    uiText->Initialize(m_vertexBuffer.Get(), m_indexBuffer.Get(), &m_vertexBufferView, &m_indexBufferView);

    uiText->m_size = XMFLOAT2(1.0f, 1.0f);

    m_elements.push_back(uiText);

    return uiText;
}

void UIManager::BeginRender()
{
    if (m_elements.size() <= 0) {
        return;
    }

    m_pCommandList->SetGraphicsRootSignature(m_uiRootSignature.GetRootSignature());   //���[�g�V�O�l�`����ݒ�
    m_pCommandList->SetPipelineState(m_uiPipelineState.GetPipelineState());           //�p�C�v���C���X�e�[�g��ݒ�

    ID3D12DescriptorHeap* pHeap = m_descriptorHeap.GetHeap();
    m_pCommandList->SetDescriptorHeaps(1, &pHeap);

    m_pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);                  //���_���𑗐M
    m_pCommandList->IASetIndexBuffer(&m_indexBufferView);                           //�C���f�b�N�X���𑗐M
    m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);    //3�p�|���S���̂�
}

void UIManager::Update()
{
    //Release()�����s����Ă���Ή��
    std::erase_if(m_elements, [&](const UIElementRef& element) {
        if (element->GetIsReleased()) {
            UINT index = element->GetDescriptorIndex();
            m_bCanAccessDescriptorIndexes[index] = true;
            return true;
        }
        return false;
        });
}

UINT UIManager::GetCanAccessDescriptorIndex()
{
    //DESCRIPTOR_COUNT�܂ł͏��ԂɎg�p����
    for (UINT i = m_nextDescriptorIndex; i < DESCRIPTOR_COUNT; i++) {
        if (m_bCanAccessDescriptorIndexes[i]) {
            m_bCanAccessDescriptorIndexes[i] = false;
            m_nextDescriptorIndex++;
            return i;
        }
    }

    //5000�܂Ŏg�p�����ꍇ�A�ēx0���猟�����ċ󂢂Ă��鍀�ڂ��擾
    for (UINT i = 0; i < DESCRIPTOR_COUNT; i++) {
        if (m_bCanAccessDescriptorIndexes[i]) {
            m_bCanAccessDescriptorIndexes[i] = false;
            m_nextDescriptorIndex++;
            return i;
        }
    }

    printf("UI�p��Descriptor�̋󂫂�����܂���B\n");
    return 0;
}

void UIManager::CreateVertexBuffer()
{
    //-1.0f�`1.0f��NDC���W
    UIVertex vertices[] =
    {
        { {-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f} }, //����
        { { 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f} }, //�E��
        { {-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f} }, //����
        { { 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f} }  //�E��
    };

    const UINT bufferSize = sizeof(vertices);
    //�q�[�v�ݒ�
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    //���_�o�b�t�@�̃��\�[�X
    CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    //�f�o�C�X�ō쐬
    HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertexBuffer));

    if (FAILED(hr)) {
        printf("���_�o�b�t�@�̐����Ɏ��s���܂����B%1xl\n", hr);
    }

    //���_�f�[�^��GPU�ɑ��M
    void* vertexDataBegin;
    m_vertexBuffer->Map(0, nullptr, &vertexDataBegin);
    memcpy(vertexDataBegin, vertices, bufferSize);
    m_vertexBuffer->Unmap(0, nullptr);

    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(UIVertex);
    m_vertexBufferView.SizeInBytes = bufferSize;
}

void UIManager::CreateIndexBuffer()
{
    uint16_t indices[] = { 0, 1, 2, 2, 1, 3 };

    //�C���f�b�N�X�o�b�t�@�̍쐬
    const UINT indexBufferSize = sizeof(indices);

    //�q�[�v�ݒ�
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    //�C���f�b�N�X�o�b�t�@�̃��\�[�X
    CD3DX12_RESOURCE_DESC indexBuffer = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

    HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &indexBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indexBuffer));

    if (FAILED(hr)) {
        printf("�C���f�b�N�X�o�b�t�@�̐����Ɏ��s���܂����B\n");
    }

    //�C���f�b�N�X�f�[�^��GPU�ɑ��M
    void* indexDataBegin;
    m_indexBuffer->Map(0, nullptr, &indexDataBegin);
    memcpy(indexDataBegin, indices, indexBufferSize);
    m_indexBuffer->Unmap(0, nullptr);

    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    m_indexBufferView.SizeInBytes = indexBufferSize;
}
