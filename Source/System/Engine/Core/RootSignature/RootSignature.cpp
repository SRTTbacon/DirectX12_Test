#include "RootSignature.h"

RootSignature::RootSignature(ID3D12Device* device)
    : m_device(device) {
}

RootSignature::~RootSignature() {
}

void RootSignature::Create() {
    // ���[�g�p�����[�^�̐ݒ�
    CD3DX12_ROOT_PARAMETER rootParameters[1];

    // �萔�o�b�t�@�̎Q�Ƃ��쐬
    rootParameters[0].InitAsConstantBufferView(0);

    // ���[�g�V�O�l�`���̐ݒ�
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // ���[�g�V�O�l�`���̃V���A���C�Y
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr)) {
        if (error) {
            OutputDebugStringA((char*)error->GetBufferPointer());
        }
        return;
    }

    // ���[�g�V�O�l�`���̍쐬
    hr = m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
    if (FAILED(hr)) {
        // �G���[�n���h�����O
        OutputDebugStringA("Failed to create root signature.");
        return;
    }
}
