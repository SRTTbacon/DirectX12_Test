#pragma once

#include <d3dcompiler.h>

#include "..\\..\\Model\\Model.h"

class RaytracingProcess
{
public:
	//BLAS情報の管理構造
	//BLAS = BottomLevelAccelerationStructureの略
	struct BLAS {
		ComPtr<ID3D12Resource> blasBuffer;
		ComPtr<ID3D12Resource> scratchBuffer;
	};

public:
	RaytracingProcess(ID3D12Device6* pDevice, ID3D12GraphicsCommandList4* pCommandList);

	HRESULT CreateRaytracingPipeline();

	HRESULT CreateBLAS(Model* pModel);

	std::vector<BLAS> m_blasList; //作成されたBLASのリスト

private:
	ID3D12Device6* m_pDevice;
	ID3D12GraphicsCommandList4* m_pCommandList;

	ComPtr<ID3D12StateObject> m_pRaytracingStateObject;		//レイトレーシング用のパイプラインステート

	ComPtr<ID3D12Resource> g_blasScratchBuffer;				//Scratchバッファ
	ComPtr<ID3D12Resource> g_blasOutputBuffer;				//BLASバッファ
};
