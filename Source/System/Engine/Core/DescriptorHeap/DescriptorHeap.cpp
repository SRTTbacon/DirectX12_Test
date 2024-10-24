#include "DescriptorHeap.h"
#include "..\\Texture2D\\\Texture2D.h"
#include <d3dx12.h>
#include "..\\..\\Engine.h"

const UINT HANDLE_MAX = 512;

DescriptorHeap::DescriptorHeap()
	: m_pHandle(nullptr)
{
	m_pHandles.clear();
	m_pHandles.reserve(HANDLE_MAX);

	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.NodeMask = 1;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = HANDLE_MAX;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	auto device = g_Engine->Device();

	//ディスクリプタヒープを生成
	auto hr = device->CreateDescriptorHeap(
		&desc,
		IID_PPV_ARGS(m_pHeap.ReleaseAndGetAddressOf()));

	if (FAILED(hr))
	{
		m_IsValid = false;
		return;
	}

	m_IncrementSize = device->GetDescriptorHandleIncrementSize(desc.Type); // ディスクリプタヒープ1個のメモリサイズを返す
	m_IsValid = true;
}

ID3D12DescriptorHeap* DescriptorHeap::GetHeap()
{
	return m_pHeap.Get();
}

DescriptorHandle* DescriptorHeap::Register(Texture2D* texture)
{
	size_t count = m_pHandles.size();
	if (HANDLE_MAX <= count)
	{
		return nullptr;
	}

	DescriptorHandle* pHandle = new DescriptorHandle();

	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = m_pHeap->GetCPUDescriptorHandleForHeapStart(); // ディスクリプタヒープの最初のアドレス
	handleCPU.ptr += m_IncrementSize * static_cast<unsigned long long>(count); // 最初のアドレスからcount番目が今回追加されたリソースのハンドル

	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = m_pHeap->GetGPUDescriptorHandleForHeapStart(); // ディスクリプタヒープの最初のアドレス
	handleGPU.ptr += m_IncrementSize * static_cast<unsigned long long>(count); // 最初のアドレスからcount番目が今回追加されたリソースのハンドル

	pHandle->HandleCPU = handleCPU;
	pHandle->HandleGPU = handleGPU;
	pHandle->UseCount = 1;

	ID3D12Device* device = g_Engine->Device();
	ID3D12Resource* resource = texture->Resource();
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = texture->ViewDesc();
	device->CreateShaderResourceView(resource, &desc, pHandle->HandleCPU); // シェーダーリソースビュー作成

	m_pHandles.push_back(pHandle);

	m_pHandle = pHandle;
	return pHandle; // ハンドルを返す
}

DescriptorHandle* DescriptorHeap::Register()
{
	size_t count = m_pHandles.size();
	if (HANDLE_MAX <= count)
	{
		return nullptr;
	}

	DescriptorHandle* pHandle = new DescriptorHandle();

	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = m_pHeap->GetCPUDescriptorHandleForHeapStart(); // ディスクリプタヒープの最初のアドレス
	handleCPU.ptr += m_IncrementSize * static_cast<unsigned long long>(count); // 最初のアドレスからcount番目が今回追加されたリソースのハンドル

	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = m_pHeap->GetGPUDescriptorHandleForHeapStart(); // ディスクリプタヒープの最初のアドレス
	handleGPU.ptr += m_IncrementSize * static_cast<unsigned long long>(count); // 最初のアドレスからcount番目が今回追加されたリソースのハンドル

	pHandle->HandleCPU = handleCPU;
	pHandle->HandleGPU = handleGPU;
	pHandle->UseCount = 1;

	m_pHandles.push_back(pHandle);
	return pHandle; // ハンドルを返す
}
