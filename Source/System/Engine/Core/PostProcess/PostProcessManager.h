#pragma once

#include "..\\PipelineState\\PipelineState.h"
#include <DirectXMath.h>

class PostProcessManager
{
public:
	PostProcessManager();
	~PostProcessManager();

	void Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList);

	void Render(ID3D12Resource* pRenderSource);

private:
	struct Vertex
	{
		DirectX::XMFLOAT3 position; // 位置
		DirectX::XMFLOAT2 uv;       // テクスチャ座標
	};

	struct NoiseSettings
	{
		float time;
		float glitchIntensity;    // グリッチの強さ
		float distortionStrength; // UVの歪み
		float blockSize;          // ブロックノイズのサイズ
		float noiseStrength;      // 通常ノイズの強さ
	};

	ID3D12Device* m_pDevice;
	ID3D12GraphicsCommandList* m_pCommandList;

	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;

	DescriptorHeap m_descriptorHeap;

	ComPtr<ID3D12Resource> m_vertexBuffer;
	ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;      //頂点バッファのデータ内容とサイズを保持
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;        //インデックスバッファのデータ内容とサイズを保持

	float m_time;

	void CreateVertexBuffer();
	void CreateIndexBuffer();
};
