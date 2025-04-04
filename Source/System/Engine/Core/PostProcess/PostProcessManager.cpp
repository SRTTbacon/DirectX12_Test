#include "PostProcessManager.h"

PostProcessManager::PostProcessManager()
	: m_pDevice(nullptr)
	, m_pCommandList(nullptr)
	, m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
    , m_vertexBufferView(D3D12_VERTEX_BUFFER_VIEW())
    , m_indexBufferView(D3D12_INDEX_BUFFER_VIEW())
    , m_time(0.0f)
{
}

PostProcessManager::~PostProcessManager()
{
    if (m_pPipelineState) {
        delete m_pPipelineState;
        m_pPipelineState = nullptr;
    }
    if (m_pRootSignature) {
        delete m_pRootSignature;
        m_pRootSignature = nullptr;
    }
}

void PostProcessManager::Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList)
{
	m_pDevice = pDevice;
	m_pCommandList = pCommandList;

	m_pRootSignature = new RootSignature(pDevice, ShaderKinds::PostProcessShader);
	m_pPipelineState = new PipelineState(pDevice, m_pRootSignature);

    m_descriptorHeap.Initialize(pDevice, 1);

    CreateVertexBuffer();
    CreateIndexBuffer();
}

void PostProcessManager::Render(ID3D12Resource* pRenderSource)
{
    m_descriptorHeap.SetResource(0, pRenderSource);

    m_time += 0.016f;

    NoiseSettings settings{};
    settings.time = m_time;         //�o�ߎ���
    settings.glitchIntensity = 1.3f; //�m�C�Y�̋��x
    settings.distortionStrength = 0.4f;
    settings.blockSize = 200.0f;
    settings.noiseStrength = 0.5f;

    // ���[�g�V�O�l�`����PSO��ݒ�
    m_pCommandList->SetGraphicsRootSignature(m_pRootSignature->GetRootSignature());
    m_pCommandList->SetPipelineState(m_pPipelineState->GetPipelineState());

    ID3D12DescriptorHeap* pHeap = m_descriptorHeap.GetHeap();
    m_pCommandList->SetDescriptorHeaps(1, &pHeap);

    //�G�t�F�N�g�^�C�v��ݒ�
    m_pCommandList->SetGraphicsRoot32BitConstants(0, sizeof(NoiseSettings) / 4, &settings, 0);

    //���ԃe�N�X�`����SRV�Ƃ��Đݒ�
    m_pCommandList->SetGraphicsRootDescriptorTable(1, pHeap->GetGPUDescriptorHandleForHeapStart());

    m_pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);      //���_���𑗐M
    m_pCommandList->IASetIndexBuffer(&m_indexBufferView);               //�C���f�b�N�X���𑗐M

    // �t���X�N���[���N���b�h�`��
    m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void PostProcessManager::CreateVertexBuffer()
{
    //-1.0f�`1.0f��NDC���W
    Vertex vertices[] =
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
    m_vertexBufferView.StrideInBytes = sizeof(Vertex);
    m_vertexBufferView.SizeInBytes = bufferSize;
}

void PostProcessManager::CreateIndexBuffer()
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
