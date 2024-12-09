#pragma once

#include "..\\Model\\Model.h"

constexpr int SHADOW_SIZE = 8192;

class ZShadow
{
public:
	ZShadow(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList);

	void BeginMapping();

	inline ID3D12Resource* GetZBuffer() const
	{
		return m_pZBufferTexture.Get();
	}

private:
	ID3D12Device* m_pDevice;
	ID3D12GraphicsCommandList* m_pCommandList;
	ComPtr<ID3D12Resource> m_pZBufferTexture;
	ComPtr<ID3D12DescriptorHeap> m_pShadowDSVHeap;
	ComPtr<ID3D12DescriptorHeap> m_pShadowSRVHeap;

	RootSignature* m_pShadowRootSignature = nullptr;		//影用のルートシグネチャ
	PipelineState* m_pShadowPipelineState = nullptr;		//影用のパイプラインステート

	void CreateBuffer();
};