#pragma once
#include "..\\..\\..\\ComPtr.h"
#include <d3dx12.h>
#include <string>

class PipelineState
{
public:
	PipelineState(ID3D12Device* device);
	~PipelineState();

	void SetVS(std::wstring filePath); // 頂点シェーダーを設定
	void SetPS(std::wstring filePath); // ピクセルシェーダーを設定
	void CreatePipelineState();

	ID3D12PipelineState* Get() const { return m_pipelineState.Get(); }
	bool IsValid() const { return m_bInited; };

private:
	ID3D12Device* m_pDevice;

	std::wstring vsFilePath, psFilePath;

	bool m_bInited;

	ComPtr<ID3D12PipelineState> m_pipelineState = nullptr; // パイプラインステート
};
