#include "SkyBox.h"

using namespace DirectX;

//�����̂̒��_�f�[�^�i�傫�ȗ����́j
const SkyboxVertex SkyBox::skyboxVertices[] =
{
    { {-1.0f,  1.0f, -1.0f} }, // 0: ����O
    { { 1.0f,  1.0f, -1.0f} }, // 1: �E��O
    { {-1.0f, -1.0f, -1.0f} }, // 2: �����O
    { { 1.0f, -1.0f, -1.0f} }, // 3: �E���O
    { {-1.0f,  1.0f,  1.0f} }, // 4: ���㉜
    { { 1.0f,  1.0f,  1.0f} }, // 5: �E�㉜
    { {-1.0f, -1.0f,  1.0f} }, // 6: ������
    { { 1.0f, -1.0f,  1.0f} }, // 7: �E����
};
//�C���f�b�N�X�f�[�^�i�X�J�C�{�b�N�X�p�j
const uint16_t SkyBox::skyboxIndices[] =
{
    // �O��
    0, 1, 2,
    2, 1, 3,
    // �w��
    5, 4, 7,
    7, 4, 6,
    // ����
    4, 0, 6,
    6, 0, 2,
    // �E��
    1, 5, 3,
    3, 5, 7,
    // ���
    4, 5, 0,
    0, 5, 1,
    // ����
    2, 3, 6,
    6, 3, 7,
};

SkyBox::SkyBox()
    : m_pDevice(nullptr)
    , m_pCommandList(nullptr)
    , m_pCamera(nullptr)
    , m_pMaterialManager(nullptr)
    , m_pRootSignature(nullptr)
    , m_pPipelineState(nullptr)
    , m_pSkyTexture2D(nullptr)
    , m_vertexBufferView(D3D12_VERTEX_BUFFER_VIEW())
    , m_indexBufferView(D3D12_INDEX_BUFFER_VIEW())
{
}

SkyBox::~SkyBox()
{
    if (m_pRootSignature) {
        delete m_pRootSignature;
        m_pRootSignature = nullptr;
    }
    if (m_pPipelineState) {
        delete m_pPipelineState;
        m_pPipelineState = nullptr;
    }

    if (m_pSkyTexture2D) {
        delete m_pSkyTexture2D;
        m_pSkyTexture2D = nullptr;
    }
}

void SkyBox::Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, Camera* pCamera, MaterialManager* pMaterialManager)
{
    m_pDevice = pDevice;
    m_pCommandList = pCommandList;
    m_pCamera = pCamera;
    m_pMaterialManager = pMaterialManager;

    m_pRootSignature = new RootSignature(pDevice, ShaderKinds::SkyBoxShader);
    m_pPipelineState = new PipelineState(pDevice, m_pRootSignature);

    //���_�V�F�[�_�[(b0)�p�̃��\�[�X���쐬
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    CD3DX12_RESOURCE_DESC constantData = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SkyBoxContentBuffer) + 255) & ~255);
    HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &constantData, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_projBuffer));

    if (FAILED(hr)) {
        printf("�R���X�^���g�o�b�t�@�̐����Ɏ��s:�G���[�R�[�h%d\n", hr);
    }

    CreateMesh();
}

void SkyBox::SetSkyTexture(std::string ddsFile)
{
    if (m_pSkyTexture2D) {
        delete m_pSkyTexture2D;
        m_pSkyTexture2D = nullptr;
    }

    m_pSkyTexture2D = Texture2D::Get(ddsFile);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_pSkyTexture2D->Resource()->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = 1;

	m_pMaterialManager->GetDescriptorHeap()->SetResource(SKYBOX_HEAP_INDEX, m_pSkyTexture2D->Resource(), srvDesc);

    D3D12_RESOURCE_DESC texDesc = m_pSkyTexture2D->Resource()->GetDesc();
    if (texDesc.DepthOrArraySize != 6 || texDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
        printf("�L���[�u�}�b�v�e�N�X�`�������[�h�ł��܂���ł����B\n");
    }
}

void SkyBox::LateUpdate()
{
    if (!m_pSkyTexture2D) {
        return;
    }

	//�J�����̃r���[�s�񂩂畽�s�ړ�����������
    SkyBoxContentBuffer buffer{};
	XMMATRIX viewMatrix = m_pCamera->m_viewMatrix;
    viewMatrix.r[3].m128_f32[0] = 0.0f;
    viewMatrix.r[3].m128_f32[1] = 0.0f;
    viewMatrix.r[3].m128_f32[2] = 0.0f;

	//VP�s����X�V
    buffer.viewProj = XMMatrixMultiply(viewMatrix, m_pCamera->m_projMatrix);

	//�萔�o�b�t�@�Ƀf�[�^����������
	void* p0;
	HRESULT hr = m_projBuffer->Map(0, nullptr, &p0);
	if (p0) {
		memcpy(p0, &buffer, sizeof(SkyBoxContentBuffer));
		m_projBuffer->Unmap(0, nullptr);
	}
}

void SkyBox::Draw()
{
    if (!m_pSkyTexture2D) {
        return;
    }

	m_pCommandList->SetGraphicsRootSignature(m_pRootSignature->GetRootSignature());   //���[�g�V�O�l�`����ݒ�
	m_pCommandList->SetPipelineState(m_pPipelineState->GetPipelineState());           //�p�C�v���C���X�e�[�g��ݒ�

	m_pCommandList->SetGraphicsRootConstantBufferView(0, m_projBuffer->GetGPUVirtualAddress()); //���f���̈ʒu�֌W�𑗐M

	m_pCommandList->SetGraphicsRootDescriptorTable(1, m_pMaterialManager->GetDescriptorHeap()->GetGpuDescriptorHandle(SKYBOX_HEAP_INDEX));        //���_�V�F�[�_�[�̃V�F�C�v�L�[

    m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);    //3�p�|���S���̂�
    m_pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);                  //���_���𑗐M
	m_pCommandList->IASetIndexBuffer(&m_indexBufferView);                           //�C���f�b�N�X���𑗐M

	m_pCommandList->DrawIndexedInstanced(std::size(skyboxIndices), 1, 0, 0, 0);     //�`��
}

void SkyBox::CreateMesh()
{
    CreateVertexBuffer();
    CreateIndexBuffer();
}

//���_�o�b�t�@�̍쐬
void SkyBox::CreateVertexBuffer()
{
    //�q�[�v�ݒ�
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    //���_�o�b�t�@�̃��\�[�X
    const UINT vertexBufferSize = static_cast<UINT>(std::size(skyboxVertices) * sizeof(SkyboxVertex));
    CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

    //�f�o�C�X�ō쐬
    HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertexBuffer));

    if (FAILED(hr)) {
        printf("���_�o�b�t�@�̐����Ɏ��s���܂����B%1xl\n", hr);
    }

    //���_�f�[�^��GPU�ɑ��M
    void* vertexDataBegin;
    m_vertexBuffer->Map(0, nullptr, &vertexDataBegin);
    memcpy(vertexDataBegin, skyboxVertices, vertexBufferSize);
    m_vertexBuffer->Unmap(0, nullptr);

    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(SkyboxVertex);
    m_vertexBufferView.SizeInBytes = vertexBufferSize;
}

//�C���f�b�N�X�o�b�t�@�̍쐬
void SkyBox::CreateIndexBuffer()
{
    //�q�[�v�ݒ�
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    //�C���f�b�N�X�o�b�t�@�̍쐬
    const UINT indexBufferSize = static_cast<UINT>(std::size(skyboxIndices) * sizeof(uint16_t));

    //�C���f�b�N�X�o�b�t�@�̃��\�[�X
    CD3DX12_RESOURCE_DESC indexBuffer = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

    HRESULT hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &indexBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indexBuffer));

    if (FAILED(hr)) {
        printf("�C���f�b�N�X�o�b�t�@�̐����Ɏ��s���܂����B\n");
    }

    //�C���f�b�N�X�f�[�^��GPU�ɑ��M
    void* indexDataBegin;
    m_indexBuffer->Map(0, nullptr, &indexDataBegin);
    memcpy(indexDataBegin, skyboxIndices, indexBufferSize);
    m_indexBuffer->Unmap(0, nullptr);

    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    m_indexBufferView.SizeInBytes = indexBufferSize;
}
