#pragma once
#include <d3dx12.h>
#include <string>
#include "..\\RootSignature\\RootSignature.h"
#include "..\\SharedStruct\\SharedStruct.h"

class PipelineState
{
public:
	PipelineState(ID3D12Device* device);
	~PipelineState();

	void SetVS(std::wstring filePath); // ���_�V�F�[�_�[��ݒ�
	void SetPS(std::wstring filePath); // �s�N�Z���V�F�[�_�[��ݒ�
	void CreatePipelineState(RootSignature* pRootSignature);

	ID3D12PipelineState* Get() const { return m_pipelineState.Get(); }
	bool IsValid() const { return m_bInited; };

private:
	ID3D12Device* m_pDevice;

	std::wstring vsFilePath, psFilePath;

	bool m_bInited;

	ComPtr<ID3D12PipelineState> m_pipelineState = nullptr; // �p�C�v���C���X�e�[�g
};
