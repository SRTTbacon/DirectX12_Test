#pragma once
#include "..\\..\\..\\ComPtr.h"
#include "..\\..\\..\\Main\\Utility.h"
#include <string>
#include <unordered_map>
#include <d3dx12.h>

class Texture2D;

struct TextureManage
{
	UINT uniqueID;			//���j�[�NID
	UINT refCount;			//�Q�ƃJ�E���g
	bool bSimpleTex;		//�P�F
	ComPtr<ID3D12Resource> pResource; //���\�[�X
};

class Texture2D
{
public:
	~Texture2D();

	static Texture2D* Get(std::string path);	//string�Ŏ󂯎�����p�X����e�N�X�`����ǂݍ���
	static Texture2D* Get(std::wstring path);	//wstring�Ŏ󂯎�����p�X����e�N�X�`����ǂݍ���
	//�P�F�e�N�X�`���𐶐�
	//���� : r, g, b 0.0f�`1.0f�͈̔͂ŐF���w��
	static Texture2D* GetColor(float r, float g, float b);
	static ID3D12Resource* GetDefaultResource(DXGI_FORMAT format, bool bPixelShader, size_t width, size_t height);

	bool IsValid() const; //����ɓǂݍ��܂�Ă��邩�ǂ�����Ԃ�
	bool IsSimpleTex() const;

	ID3D12Resource* Resource(); //���\�[�X��Ԃ�
	D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc(); //�V�F�[�_�[���\�[�X�r���[�̐ݒ��Ԃ�

private:
	TextureManage* m_pManage;	//�e�N�X�`���Ǘ��\����

	Texture2D(std::string path);
	Texture2D(std::wstring path);
	Texture2D(TextureManage* pTextureManage);

	bool Load(std::string& path);
	bool Load(std::wstring& path);

	Texture2D(const Texture2D&) = delete;
	void operator = (const Texture2D&) = delete;
};
