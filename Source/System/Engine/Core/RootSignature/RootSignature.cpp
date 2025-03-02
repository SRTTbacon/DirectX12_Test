#include "RootSignature.h"

RootSignature::RootSignature(ID3D12Device* device, ShaderKinds shaderKind)
	: m_shaderKind(shaderKind)
	, m_rootParamSize(0)
	, m_pDiscriptorRange(nullptr)
{
	auto flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;	//�A�v���P�[�V�����̓��̓A�Z���u�����g�p����
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;			//�h���C���V�F�[�_�[�̃��[�g�V�O�l�`���ւ̃A�N�Z�X�����ۂ���
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;				//�n���V�F�[�_�[�̃��[�g�V�O�l�`���ւ̃A�N�Z�X�����ۂ���
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;			//�W�I���g���V�F�[�_�[�̃��[�g�V�O�l�`���ւ̃A�N�Z�X�����ۂ���

	CD3DX12_ROOT_PARAMETER* rootParam = GetRootParameter();
	if (!rootParam) {
		printf("ShaderKind���������ݒ肳��Ă��܂���B\n");
		return;
	}

	//�X�^�e�B�b�N�T���v���[�̐ݒ�
	int samplerCount = 0;
	CD3DX12_STATIC_SAMPLER_DESC* samplers = nullptr;

	//�e�p�̃V�F�[�_�[�ȊO
	if (shaderKind != ShaderKinds::ShadowShader) {
		samplerCount = 2;
		samplers = new CD3DX12_STATIC_SAMPLER_DESC[samplerCount];
		samplers[0] = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
		samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		samplers[1] = CD3DX12_STATIC_SAMPLER_DESC(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		samplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	}

	//���[�g�V�O�j�`���̐ݒ�i���[�g�p�����[�^�[�ƃX�^�e�B�b�N�T���v���[������j
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = m_rootParamSize;		//���[�g�p�����[�^�[�̌��������
	desc.NumStaticSamplers = samplerCount;		//�T���v���[�̌��������
	desc.pParameters = rootParam;				//���[�g�p�����[�^�[�̃|�C���^�������
	desc.pStaticSamplers = samplers;			//�T���v���[�̃|�C���^������
	desc.Flags = flag;							//�t���O��ݒ�

	ComPtr<ID3DBlob> pBlob;
	ComPtr<ID3DBlob> pErrorBlob;

	//�V���A���C�Y
	HRESULT hr = D3D12SerializeRootSignature(
		&desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		pBlob.GetAddressOf(),
		pErrorBlob.GetAddressOf());
	if (FAILED(hr))
	{
		if (pErrorBlob) {
			printf("���[�g�V�O�l�`���V���A���C�Y�Ɏ��s - %s\n", (char*)pErrorBlob->GetBufferPointer());
		}
		else {
			printf("���[�g�V�O�l�`���V���A���C�Y�Ɏ��s\n");
		}
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
		HRESULT hr2 = device->GetDeviceRemovedReason();
		printf("���[�g�V�O�l�`���̐����Ɏ��s�B %u, %u, �V�F�[�_�[ : %d\n", (UINT)hr, (UINT)hr2, shaderKind);
		return;
	}

	if (m_pDiscriptorRange) {
		delete m_pDiscriptorRange;
		m_pDiscriptorRange = nullptr;
	}

	delete[] rootParam;
}

ID3D12RootSignature* RootSignature::GetRootSignature() const {
    return rootSignature.Get();
}

CD3DX12_ROOT_PARAMETER* RootSignature::GetRootParameter()
{
	//�{�[�������݂���V�F�[�_�[�̏ꍇ
	if (m_shaderKind == ShaderKinds::BoneShader) {
		m_rootParamSize = 8;
		CD3DX12_ROOT_PARAMETER* rootParam = new CD3DX12_ROOT_PARAMETER[m_rootParamSize];
		rootParam[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);	//���_�V�F�[�_�[��b0�̒萔�o�b�t�@��ݒ�
		rootParam[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);		//�s�N�Z���V�F�[�_�[��b0�̒萔�o�b�t�@��ݒ�

		//�e�N�X�`���p���X���b�gt0�ɐݒ�
		m_pDiscriptorRange = new CD3DX12_DESCRIPTOR_RANGE[3];
		//�V���h�E�}�b�v�p���X���b�gt2�ɐݒ� (�ǂ̃}�e���A���ł����ʂ̂��ߓƗ�)
		m_pDiscriptorRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);	//�V�F�[�_�[���\�[�X�r���[(1��)
		rootParam[2].InitAsDescriptorTable(1, &m_pDiscriptorRange[0], D3D12_SHADER_VISIBILITY_PIXEL);	//t2

		m_pDiscriptorRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);	//�V�F�[�_�[���\�[�X�r���[(2��)
		rootParam[3].InitAsDescriptorTable(1, &m_pDiscriptorRange[1], D3D12_SHADER_VISIBILITY_PIXEL);	//t0�At1
		//�V�F�C�v�L�[
		m_pDiscriptorRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);	//t0
		rootParam[4].InitAsDescriptorTable(1, &m_pDiscriptorRange[2], D3D12_SHADER_VISIBILITY_VERTEX);

		rootParam[5].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);	//���_�V�F�[�_�[��b1�̒萔�o�b�t�@��ݒ�
		rootParam[6].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_VERTEX);	//���_�V�F�[�_�[��b2�̒萔�o�b�t�@��ݒ�

		//�V�F�C�v�L�[�̃E�F�C�g�p���X���b�gt1�ɐݒ�
		rootParam[7].InitAsShaderResourceView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);

		return rootParam;
	}
	//�P�F�̃V�F�[�_�[
	else if (m_shaderKind == ShaderKinds::PrimitiveShader) {
		m_rootParamSize = 4;
		CD3DX12_ROOT_PARAMETER* rootParam = new CD3DX12_ROOT_PARAMETER[m_rootParamSize];
		rootParam[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);	//���_�V�F�[�_�[��b0�̒萔�o�b�t�@��ݒ�
		rootParam[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);		//�s�N�Z���V�F�[�_�[��b0�̒萔�o�b�t�@��ݒ�

		//�e�N�X�`���p���X���b�gt0�ɐݒ�
		m_pDiscriptorRange = new CD3DX12_DESCRIPTOR_RANGE[2];
		m_pDiscriptorRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);	//�V�F�[�_�[���\�[�X�r���[(1��)
		rootParam[2].InitAsDescriptorTable(1, &m_pDiscriptorRange[0], D3D12_SHADER_VISIBILITY_PIXEL);	//t2

		m_pDiscriptorRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);	//�V�F�[�_�[���\�[�X�r���[(2��)
		rootParam[3].InitAsDescriptorTable(1, &m_pDiscriptorRange[1], D3D12_SHADER_VISIBILITY_PIXEL);	//t0�At1

		return rootParam;
	}
	//�e�݂̂̃V�F�[�_�[
	else if (m_shaderKind == ShaderKinds::ShadowShader) {
		m_rootParamSize = 5;
		CD3DX12_ROOT_PARAMETER* rootParam = new CD3DX12_ROOT_PARAMETER[m_rootParamSize];
		//�萔�o�b�t�@�����[�g�p�����[�^�Ƃ��Đݒ� (���b�V�����Ƃ̃��[���h�ϊ��s��p)
		rootParam[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);	//���_�V�F�[�_�[��b0�̒萔�o�b�t�@��ݒ�
		rootParam[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);	//���_�V�F�[�_�[��b1�̒萔�o�b�t�@��ݒ�
		rootParam[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_VERTEX);	//���_�V�F�[�_�[��b2�̒萔�o�b�t�@��ݒ�

		//�V�F�C�v�L�[
		m_pDiscriptorRange = new CD3DX12_DESCRIPTOR_RANGE();
		m_pDiscriptorRange->Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);	//���_�V�F�[�_�[(1��)
		rootParam[3].InitAsDescriptorTable(1, m_pDiscriptorRange, D3D12_SHADER_VISIBILITY_VERTEX);	//t0

		//�V�F�C�v�L�[�̃E�F�C�g�p���X���b�gt1�ɐݒ�
		rootParam[4].InitAsShaderResourceView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);

		return rootParam;
	}
	return nullptr;
}
