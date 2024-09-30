#pragma once

#include <d3d12.h>

// "���S��"COM�|�C���^�̉�������}�N��
#define SAFE_RELEASE(p) if(p){ p->Release(); p=NULL; }

// "���S��"�z��̉�������}�N��
#define SAFE_DELETE(p) if(p){ delete[] p; p=NULL; }

using namespace DirectX;

// �摜�̃N���X
class Texture2D
{
	// ���
	enum
	{
		STATE_INIT = 0,
		STATE_COPY,
		STATE_COPY_WAIT,
		STATE_IDLE,
	};

public:
	Texture2D();
	~Texture2D();

	// �X�V
	void Update(ID3D12GraphicsCommandList* pCommandList);

	// �e�N�X�`���[�̍쐬
	bool CreateTexture(ID3D12Device* pDevice, const wchar_t* pFile);
	// �V�F�[�_�[���\�[�X�r���[�̍쐬
	void CreateSRV(ID3D12Device* pDevice, D3D12_CPU_DESCRIPTOR_HANDLE handle);

	// �R�s�[�ς݂�
	bool IsCopied() const { return m_State > STATE_INIT; }


private:

	// �e�N�X�`�������R�s�[
	void CopyTexture(ID3D12GraphicsCommandList* pCommandList);

	void CreateResource(ID3D12Device* pDevice, const Image* pImage, TexMetadata& metadata);


private:
	int		m_Count;
	int		m_State;

	// ���\�[�X���
	size_t	m_RowPitch;

	ID3D12Resource* m_pTexture;
	ID3D12Resource* m_pTextureUpload;
};