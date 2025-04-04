#pragma once

#include <d3dx12.h>
#include <thread>
#include <condition_variable>
#include <mutex>

#include "..\\..\\..\\ComPtr.h"

//���\�[�X�R�s�[�p�R�}���h���X�g���Ǘ�
//�}���`�X���b�h�Ή��̂��߁A�g�p����ۂ�std::thread������s���邱�Ƃ𐄏�

class ResourceCopy
{
public:
	ResourceCopy();

	//�A�b�v���[�h�p�̃��\�[�X���쐬
	//����͊e�X�ōs��
	ID3D12Resource* CreateUploadBuffer(UINT64 bufferSize);

	//������ (�G���W��������s)
	void Initialize(ID3D12Device* pDevice);

	//�R�s�[�p�̃R�}���h���X�g�������� (���X���b�h�Ŏg�p���̏ꍇ�͑ҋ@�����)
	void BeginCopyResource();

	//�R�}���h�����s
	void EndCopyResource();

public:
	inline ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList.Get(); }

private:
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	ComPtr<ID3D12CommandAllocator> m_allocator;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12Fence> m_fence;

	ID3D12Device* m_pDevice;

	HANDLE m_eventHandle;

	std::mutex m_mutex;
	std::condition_variable m_condition;

	UINT64 m_fenceValue;

	bool m_bCanExecute;
};

//�ǂ�����ł��Q�Ƃ��邽�߃O���[�o���ϐ�
extern ResourceCopy* g_resourceCopy;
