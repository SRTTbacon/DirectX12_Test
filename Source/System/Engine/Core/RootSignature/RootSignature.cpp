#include "RootSignature.h"

RootSignature::RootSignature(ID3D12Device* device, ShaderKinds shaderKind)
	: m_shaderKind(shaderKind)
	, m_rootParamSize(0)
{
	auto flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;	//�A�v���P�[�V�����̓��̓A�Z���u�����g�p����
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;			//�h���C���V�F�[�_�[�̃��[�g�V�O�l�`���ւ̃A�N�Z�X�����ۂ���
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;				//�n���V�F�[�_�[�̃��[�g�V�O�l�`���ւ̃A�N�Z�X�����ۂ���
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;			//�W�I���g���V�F�[�_�[�̃��[�g�V�O�l�`���ւ̃A�N�Z�X�����ۂ���

	CD3DX12_ROOT_PARAMETER* rootParam = GetRootParameter();

	//�X�^�e�B�b�N�T���v���[�̐ݒ�
	auto sampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	//���[�g�V�O�j�`���̐ݒ�i���[�g�p�����[�^�[�ƃX�^�e�B�b�N�T���v���[������j
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = m_rootParamSize;		//���[�g�p�����[�^�[�̌��������
	desc.NumStaticSamplers = 1;					//�T���v���[�̌��������
	desc.pParameters = rootParam;				//���[�g�p�����[�^�[�̃|�C���^�������
	desc.pStaticSamplers = &sampler;			//�T���v���[�̃|�C���^������
	desc.Flags = flag;							//�t���O��ݒ�

	ComPtr<ID3DBlob> pBlob;
	ComPtr<ID3DBlob> pErrorBlob;

	//�V���A���C�Y
	auto hr = D3D12SerializeRootSignature(
		&desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		pBlob.GetAddressOf(),
		pErrorBlob.GetAddressOf());
	if (FAILED(hr))
	{
		printf("���[�g�V�O�l�`���V���A���C�Y�Ɏ��s\n");
		return;
	}

	//���[�g�V�O�l�`������
	hr = device->CreateRootSignature(
		0,												//GPU����������ꍇ�̃m�[�h�}�X�N�i��{1�̂���0���w��j
		pBlob->GetBufferPointer(),						//�V���A���C�Y�����f�[�^�̃|�C���^
		pBlob->GetBufferSize(),							//�V���A���C�Y�����f�[�^�̃T�C�Y
		IID_PPV_ARGS(rootSignature.GetAddressOf()));	//���[�g�V�O�j�`���i�[��̃|�C���^
	if (FAILED(hr))
	{
		printf("���[�g�V�O�l�`���̐����Ɏ��s\n");
		return;
	}

	if (m_pTableRange1)
		delete[] m_pTableRange1;
	delete[] rootParam;
}

ID3D12RootSignature* RootSignature::GetRootSignature() const {
    return rootSignature.Get();
}

CD3DX12_ROOT_PARAMETER* RootSignature::GetRootParameter()
{
	//�{�[�������݂���V�F�[�_�[�̏ꍇ
	if (m_shaderKind == ShaderKinds::BoneShader) {
		m_rootParamSize = 6;
		CD3DX12_ROOT_PARAMETER* rootParam = new CD3DX12_ROOT_PARAMETER[m_rootParamSize];
		rootParam[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // b0�̒萔�o�b�t�@��ݒ�A�S�ẴV�F�[�_�[���猩����悤�ɂ���
		rootParam[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX); // b1�̒萔�o�b�t�@��ݒ�A�S�ẴV�F�[�_�[���猩����悤�ɂ���
		rootParam[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_VERTEX); // b2�̒萔�o�b�t�@��ݒ�A�S�ẴV�F�[�_�[���猩����悤�ɂ���

		//�V�F�C�v�L�[�p���X���b�gt1�ɐݒ�
		rootParam[3].InitAsShaderResourceView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		//�V�F�C�v�L�[�̃E�F�C�g�p���X���b�gt2�ɐݒ�
		rootParam[4].InitAsShaderResourceView(2, 0, D3D12_SHADER_VISIBILITY_VERTEX);

		m_pTableRange1 = new CD3DX12_DESCRIPTOR_RANGE[1];				//�f�B�X�N���v�^�e�[�u��
		//�e�N�X�`���p���X���b�gt0�ɐݒ�
		m_pTableRange1[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);	//�V�F�[�_�[���\�[�X�r���[
		rootParam[5].InitAsDescriptorTable(1, m_pTableRange1, D3D12_SHADER_VISIBILITY_PIXEL);

		return rootParam;
	}
	//�P�F�̃V�F�[�_�[
	else if (m_shaderKind == ShaderKinds::PrimitiveShader)
	{
		m_rootParamSize = 1;
		CD3DX12_ROOT_PARAMETER* rootParam = new CD3DX12_ROOT_PARAMETER[m_rootParamSize];
		rootParam[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // b0�̒萔�o�b�t�@��ݒ�A�S�ẴV�F�[�_�[���猩����悤�ɂ���
		
		return rootParam;
	}
	return nullptr;
}
