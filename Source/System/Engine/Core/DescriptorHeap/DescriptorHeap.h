#pragma once
#include "..\\..\\..\\ComPtr.h"
#include <d3dx12.h>
#include <vector>

class ConstantBuffer;
class Texture2D;

class DescriptorHandle
{
public:
	D3D12_CPU_DESCRIPTOR_HANDLE HandleCPU;
	D3D12_GPU_DESCRIPTOR_HANDLE HandleGPU;
	BYTE UseCount;
};

class DescriptorHeap
{
public:
	DescriptorHeap(); // �R���X�g���N�^�Ő�������
	ID3D12DescriptorHeap* GetHeap(); // �f�B�X�N���v�^�q�[�v��Ԃ�
	DescriptorHandle* Register(Texture2D* texture); // �e�N�X�`���[���f�B�X�N���v�^�q�[�v�ɓo�^���A�n���h����Ԃ�

private:
	bool m_IsValid = false; // �����ɐ����������ǂ���
	UINT m_IncrementSize = 0;
	ComPtr<ID3D12DescriptorHeap> m_pHeap = nullptr; // �f�B�X�N���v�^�q�[�v�{��
	std::vector<DescriptorHandle*> m_pHandles; // �o�^����Ă���n���h��
};

static std::vector<DescriptorHandle*> g_materials;	//�e�N�X�`���p�̃n���h���ꗗ