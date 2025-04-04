#pragma once
#include "..\\..\\..\\ComPtr.h"
#include "..\\..\\..\\Main\\Utility.h"
#include <string>
#include <unordered_map>
#include <d3dx12.h>
#include <DirectXTex.h>
#pragma comment(lib, "DirectXTex.lib")

class Texture2D;

struct TextureManage
{
	UINT uniqueID;			//���j�[�NID
	bool bSimpleTex;		//�P�F
	ID3D12Resource* pResource; //���\�[�X

	TextureManage();
	~TextureManage();
};

class Texture2D
{
public:
	~Texture2D();

	static Texture2D* Get(std::string path);		//string�Ŏ󂯎�����p�X����e�N�X�`����ǂݍ���
	static Texture2D* Get(std::wstring path);		//wstring�Ŏ󂯎�����p�X����e�N�X�`����ǂݍ���
	static Texture2D* GetDDS(std::string path);		//string�Ŏ󂯎�����p�X����dds�e�N�X�`����ǂݍ���
	static Texture2D* GetDDS(std::wstring path);	//wstring�Ŏ󂯎�����p�X����dds�e�N�X�`����ǂݍ���

	static Texture2D* Get(const char* data, size_t size);	//����������e�N�X�`����ǂݍ���
	static Texture2D* GetDDS(const char* data, size_t size);	//����������DDS�e�N�X�`����ǂݍ���

	//�P�F�e�N�X�`���𐶐�
	//���� : r, g, b 0.0f�`1.0f�͈̔͂ŐF���w��
	static Texture2D* GetColor(float r, float g, float b, float a = 1.0f);

	bool IsValid() const; //����ɓǂݍ��܂�Ă��邩�ǂ�����Ԃ�
	bool IsSimpleTex() const;

	ID3D12Resource* Resource(); //���\�[�X��Ԃ�
	D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc(); //�V�F�[�_�[���\�[�X�r���[�̐ݒ��Ԃ�

private:
	static std::vector<std::shared_ptr<TextureManage>> s_textures;

	std::shared_ptr<TextureManage> m_manage;	//�e�N�X�`���Ǘ��\����

	Texture2D(std::string path, bool bDDSFile);
	Texture2D(std::wstring path, bool bDDSFile);
	Texture2D(const char* data, size_t size, bool bDDSFile);
	Texture2D(std::shared_ptr<TextureManage> pTextureManage);

	bool Load(std::string& path, bool bDDSFile);
	bool Load(std::wstring& path, bool bDDSFile);
	bool Load(const char* data, size_t size, bool bDDSFile);

	bool Load(UINT uniqueID, DirectX::TexMetadata& meta, DirectX::ScratchImage& image);

	Texture2D(const Texture2D&) = delete;
	void operator = (const Texture2D&) = delete;

	static ID3D12Resource* GetDefaultResource(DXGI_FORMAT format, size_t width, size_t height);
};
