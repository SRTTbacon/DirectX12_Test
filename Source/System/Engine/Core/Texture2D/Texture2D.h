#pragma once
#include "..\\..\\..\\ComPtr.h"
#include "..\\..\\..\\Main\\Utility.h"
#include <string>
#include <unordered_map>
#include <d3dx12.h>

class Texture2D;

struct TextureManage
{
	UINT uniqueID;			//ユニークID
	UINT refCount;			//参照カウント
	bool bSimpleTex;		//単色
	ComPtr<ID3D12Resource> pResource; //リソース
};

class Texture2D
{
public:
	~Texture2D();

	static Texture2D* Get(std::string path);	//stringで受け取ったパスからテクスチャを読み込む
	static Texture2D* Get(std::wstring path);	//wstringで受け取ったパスからテクスチャを読み込む
	//単色テクスチャを生成
	//引数 : r, g, b 0.0f～1.0fの範囲で色を指定
	static Texture2D* GetColor(float r, float g, float b);
	static ID3D12Resource* GetDefaultResource(DXGI_FORMAT format, bool bPixelShader, size_t width, size_t height);

	bool IsValid() const; //正常に読み込まれているかどうかを返す
	bool IsSimpleTex() const;

	ID3D12Resource* Resource(); //リソースを返す
	D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc(); //シェーダーリソースビューの設定を返す

private:
	TextureManage* m_pManage;	//テクスチャ管理構造体

	Texture2D(std::string path);
	Texture2D(std::wstring path);
	Texture2D(TextureManage* pTextureManage);

	bool Load(std::string& path);
	bool Load(std::wstring& path);

	Texture2D(const Texture2D&) = delete;
	void operator = (const Texture2D&) = delete;
};
