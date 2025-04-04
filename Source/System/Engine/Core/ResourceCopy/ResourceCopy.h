#pragma once

#include <d3dx12.h>
#include <thread>
#include <condition_variable>
#include <mutex>

#include "..\\..\\..\\ComPtr.h"

//リソースコピー用コマンドリストを管理
//マルチスレッド対応のため、使用する際はstd::threadから実行することを推奨

class ResourceCopy
{
public:
	ResourceCopy();

	//アップロード用のリソースを作成
	//解放は各々で行う
	ID3D12Resource* CreateUploadBuffer(UINT64 bufferSize);

	//初期化 (エンジンから実行)
	void Initialize(ID3D12Device* pDevice);

	//コピー用のコマンドリストを初期化 (他スレッドで使用中の場合は待機される)
	void BeginCopyResource();

	//コマンドを実行
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

//どこからでも参照するためグローバル変数
extern ResourceCopy* g_resourceCopy;
