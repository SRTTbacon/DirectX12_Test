#include "RootSignature.h"

RootSignature::RootSignature(ID3D12Device* device)
    : m_device(device) {
}

RootSignature::~RootSignature() {
}

void RootSignature::Create() {
	auto flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // �A�v���P�[�V�����̓��̓A�Z���u�����g�p����
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS; // �h���C���V�F�[�_�[�̃��[�g�V�O�l�`���ւ�A�N�Z�X�����ۂ���
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS; // �n���V�F�[�_�[�̃��[�g�V�O�l�`���ւ�A�N�Z�X�����ۂ���
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS; // �W�I���g���V�F�[�_�[�̃��[�g�V�O�l�`���ւ�A�N�Z�X�����ۂ���


    // ���[�g�p�����[�^�̐ݒ�
    CD3DX12_ROOT_PARAMETER rootParameters[2]{};

    // �萔�o�b�t�@�̎Q�Ƃ��쐬
    rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

	CD3DX12_DESCRIPTOR_RANGE tableRange[1] = {}; // �f�B�X�N���v�^�e�[�u��
	tableRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // �V�F�[�_�[���\�[�X�r���[
	rootParameters[1].InitAsDescriptorTable(std::size(tableRange), tableRange, D3D12_SHADER_VISIBILITY_ALL);

	// �X�^�e�B�b�N�T���v���[�̐ݒ�
	auto sampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	// ���[�g�V�O�j�`���̐ݒ�i�ݒ肵�������[�g�p�����[�^�[�ƃX�^�e�B�b�N�T���v���[������j
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = std::size(rootParameters); // ���[�g�p�����[�^�[�̌��������
	desc.NumStaticSamplers = 1; // �T���v���[�̌��������
	desc.pParameters = rootParameters; // ���[�g�p�����[�^�[�̃|�C���^�������
	desc.pStaticSamplers = &sampler; // �T���v���[�̃|�C���^������
	desc.Flags = flag; // �t���O��ݒ�

	ComPtr<ID3DBlob> pBlob;
	ComPtr<ID3DBlob> pErrorBlob;

	// �V���A���C�Y
	auto hr = D3D12SerializeRootSignature(
		&desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		pBlob.GetAddressOf(),
		pErrorBlob.GetAddressOf());
	if (FAILED(hr))
	{
		printf("���[�g�V�O�l�`���V���A���C�Y�Ɏ��s");
		return;
	}
    // ���[�g�V�O�l�`���̍쐬
    hr = m_device->CreateRootSignature(0, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
    if (FAILED(hr)) {
        // �G���[�n���h�����O
        OutputDebugStringA("Failed to create root signature.");
        return;
    }
}
