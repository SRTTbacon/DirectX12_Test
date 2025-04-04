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
		DirectX::XMFLOAT3 position; // �ʒu
		DirectX::XMFLOAT2 uv;       // �e�N�X�`�����W
	};

	struct NoiseSettings
	{
		float time;
		float glitchIntensity;    // �O���b�`�̋���
		float distortionStrength; // UV�̘c��
		float blockSize;          // �u���b�N�m�C�Y�̃T�C�Y
		float noiseStrength;      // �ʏ�m�C�Y�̋���
	};

	ID3D12Device* m_pDevice;
	ID3D12GraphicsCommandList* m_pCommandList;

	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;

	DescriptorHeap m_descriptorHeap;

	ComPtr<ID3D12Resource> m_vertexBuffer;
	ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;      //���_�o�b�t�@�̃f�[�^���e�ƃT�C�Y��ێ�
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;        //�C���f�b�N�X�o�b�t�@�̃f�[�^���e�ƃT�C�Y��ێ�

	float m_time;

	void CreateVertexBuffer();
	void CreateIndexBuffer();
};
