#pragma once
#include "..\\..\\..\\ComPtr.h"
#include <string>
#include <d3dx12.h>

class DescriptorHeap;
class DescriptorHandle;

class Texture3D
{
public:
	static Texture3D* Get(std::string path); // string�Ŏ󂯎�����p�X����e�N�X�`����ǂݍ���
	static Texture3D* Get(std::wstring path); // wstring�Ŏ󂯎�����p�X����e�N�X�`����ǂݍ���
	static Texture3D* GetWhite(); // ���̒P�F�e�N�X�`���𐶐�����
	bool IsValid() const; // ����ɓǂݍ��܂�Ă��邩�ǂ�����Ԃ�

	ID3D12Resource* Resource(); // ���\�[�X��Ԃ�
	D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc(); // �V�F�[�_�[���\�[�X�r���[�̐ݒ��Ԃ�

private:
	bool m_IsValid; // ����ɓǂݍ��܂�Ă��邩
	Texture3D(std::string path);
	Texture3D(std::wstring path);
	Texture3D(ID3D12Resource* buffer);
	ComPtr<ID3D12Resource> m_pResource; // ���\�[�X
	bool Load(std::string& path);
	bool Load(std::wstring& path);

	static ID3D12Resource* GetDefaultResource(size_t width, size_t height);

	Texture3D(const Texture3D&) = delete;
	void operator = (const Texture3D&) = delete;
};