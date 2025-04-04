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
	UINT uniqueID;			//ユニークID
	bool bSimpleTex;		//単色
	ID3D12Resource* pResource; //リソース

	TextureManage();
	~TextureManage();
};

class Texture2D
{
public:
	~Texture2D();

	static Texture2D* Get(std::string path);		//stringで受け取ったパスからテクスチャを読み込む
	static Texture2D* Get(std::wstring path);		//wstringで受け取ったパスからテクスチャを読み込む
	static Texture2D* GetDDS(std::string path);		//stringで受け取ったパスからddsテクスチャを読み込む
	static Texture2D* GetDDS(std::wstring path);	//wstringで受け取ったパスからddsテクスチャを読み込む

	static Texture2D* Get(const char* data, size_t size);	//メモリからテクスチャを読み込む
	static Texture2D* GetDDS(const char* data, size_t size);	//メモリからDDSテクスチャを読み込む

	//単色テクスチャを生成
	//引数 : r, g, b 0.0f～1.0fの範囲で色を指定
	static Texture2D* GetColor(float r, float g, float b, float a = 1.0f);

	bool IsValid() const; //正常に読み込まれているかどうかを返す
	bool IsSimpleTex() const;

	ID3D12Resource* Resource(); //リソースを返す
	D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc(); //シェーダーリソースビューの設定を返す

private:
	static std::vector<std::shared_ptr<TextureManage>> s_textures;

	std::shared_ptr<TextureManage> m_manage;	//テクスチャ管理構造体

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
