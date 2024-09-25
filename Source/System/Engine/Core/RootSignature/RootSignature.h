#pragma once
#include "..\\..\\..\\ComPtr.h"
#include "..\\..\\Engine.h"

struct ID3D12RootSignature;

class RootSignature
{
public:
	RootSignature(ID3D12Device* device);
	~RootSignature();

	ID3D12RootSignature* Get() { return m_rootSignature.Get(); }
	void Create();

private:
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12RootSignature> m_rootSignature;
};
