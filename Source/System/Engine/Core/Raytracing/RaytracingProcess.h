#pragma once

#include <d3dcompiler.h>

#include "..\\..\\Model\\Model.h"

class RaytracingProcess
{
public:
	//BLAS���̊Ǘ��\��
	//BLAS = BottomLevelAccelerationStructure�̗�
	struct BLAS {
		ComPtr<ID3D12Resource> blasBuffer;
		ComPtr<ID3D12Resource> scratchBuffer;
	};

public:
	RaytracingProcess(ID3D12Device6* pDevice, ID3D12GraphicsCommandList4* pCommandList);

	HRESULT CreateRaytracingPipeline();

	HRESULT CreateBLAS(Model* pModel);

	std::vector<BLAS> m_blasList; //�쐬���ꂽBLAS�̃��X�g

private:
	ID3D12Device6* m_pDevice;
	ID3D12GraphicsCommandList4* m_pCommandList;

	ComPtr<ID3D12StateObject> m_pRaytracingStateObject;		//���C�g���[�V���O�p�̃p�C�v���C���X�e�[�g

	ComPtr<ID3D12Resource> g_blasScratchBuffer;				//Scratch�o�b�t�@
	ComPtr<ID3D12Resource> g_blasOutputBuffer;				//BLAS�o�b�t�@
};
