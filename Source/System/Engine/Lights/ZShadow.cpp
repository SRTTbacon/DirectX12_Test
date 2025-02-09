#include "ZShadow.h"

ZShadow::ZShadow()
	: m_pDevice(nullptr)
    , m_pCommandList(nullptr)
{
}

ZShadow::~ZShadow()
{
    if (m_pShadowPipelineState) {
        delete m_pShadowPipelineState;
        m_pShadowPipelineState = nullptr;
    }
    if (m_pShadowRootSignature) {
        delete m_pShadowRootSignature;
        m_pShadowRootSignature = nullptr;
    }
}

void ZShadow::Init(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList)
{
    m_pDevice = pDevice;
    m_pCommandList = pCommandList;

    CreateBuffer();

    //�e�p�̃��[�g�V�O�l�`���A�p�C�v���C���X�e�[�g�̍쐬
    m_pShadowRootSignature = new RootSignature(pDevice, ShaderKinds::ShadowShader);
    m_pShadowPipelineState = new PipelineState(pDevice, m_pShadowRootSignature);
}

void ZShadow::BeginMapping()
{
    //�[�x�X�e���V���r���[�̃f�B�X�N���v�^
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_pShadowDSVHeap->GetCPUDescriptorHandleForHeapStart();

    //�V���h�E�}�b�v�ւ̃����_�[�^�[�Q�b�g��ݒ�
    m_pCommandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);

    //�r���[�|�[�g�ƃV�U�[��`��ݒ�
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = static_cast<float>(SHADOW_SIZE);
    viewport.Height = static_cast<float>(SHADOW_SIZE);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    D3D12_RECT scissorRect = {};
    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = SHADOW_SIZE;
    scissorRect.bottom = SHADOW_SIZE;

    m_pCommandList->RSSetViewports(1, &viewport);
    m_pCommandList->RSSetScissorRects(1, &scissorRect);

    //�[�x�o�b�t�@���N���A
    m_pCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    //���[�g�V�O�l�`���ƃp�C�v���C���X�e�[�g��ݒ�
    m_pCommandList->SetGraphicsRootSignature(m_pShadowRootSignature->GetRootSignature());
    m_pCommandList->SetPipelineState(m_pShadowPipelineState->GetPipelineState());
}

void ZShadow::CreateBuffer()
{
    //�[�x�X�e���V���o�b�t�@�p�̃��\�[�X���쐬
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = SHADOW_SIZE;
    texDesc.Height = SHADOW_SIZE;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    texDesc.SampleDesc.Count = 1;       //�}���`�T���v�����O�Ȃ�
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;  //�[�x�̃N���A�p�t�H�[�}�b�g
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    HRESULT hr = m_pDevice->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&m_pZBufferTexture)
    );
    if (FAILED(hr))
    {
        printf("�V���h�E�}�b�v�p�[�x�o�b�t�@�̃��\�[�X���쐬�Ɏ��s���܂����B%ld\n", hr);
        return;
    }

    //DSV�p�f�B�X�N���v�^�q�[�v
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    hr = m_pDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_pShadowDSVHeap));
    if (FAILED(hr))
    {
        printf("DSV�p�f�B�X�N���v�^�q�[�v�̍쐬�Ɏ��s���܂����B\n");
        return;
    }

    //�[�x�X�e���V���r���[���쐬
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; //�[�x�t�H�[�}�b�g
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    m_pDevice->CreateDepthStencilView(m_pZBufferTexture.Get(), &dsvDesc, m_pShadowDSVHeap->GetCPUDescriptorHandleForHeapStart());

    //SRV�p�f�B�X�N���v�^�q�[�v
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 1;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    hr = m_pDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_pShadowSRVHeap));
    if (FAILED(hr))
    {
        printf("SRV�p�f�B�X�N���v�^�q�[�v�̍쐬�Ɏ��s���܂����B\n");
        return;
    }

    //�V�F�[�_�[���\�[�X�r���[���쐬
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;     //�V�F�[�_�[�ŗ��p�\�ȃt�H�[�}�b�g
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    m_pDevice->CreateShaderResourceView(m_pZBufferTexture.Get(), &srvDesc, m_pShadowSRVHeap->GetCPUDescriptorHandleForHeapStart());
}
