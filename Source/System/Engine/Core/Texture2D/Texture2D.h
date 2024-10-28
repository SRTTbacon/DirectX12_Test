#pragma once
#include "..\\..\\..\\ComPtr.h"
#include <string>
#include <unordered_map>
#include <d3dx12.h>

class Texture2D
{
public:
	static Texture2D* Get(std::string path);	//stringで受け取ったパスからテクスチャを読み込む
	static Texture2D* Get(std::wstring path);	//wstringで受け取ったパスからテクスチャを読み込む
	static Texture2D* GetWhite();				//白の単色テクスチャを生成する
	static std::wstring GetWideString(const std::string& str);
	bool IsValid() const; //正常に読み込まれているかどうかを返す

	ID3D12Resource* Resource(); //リソースを返す
	D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc(); //シェーダーリソースビューの設定を返す

private:
	bool m_IsValid; //正常に読み込まれているか
	Texture2D(std::string path);
	Texture2D(std::wstring path);
	Texture2D(ID3D12Resource* buffer);
	ComPtr<ID3D12Resource> m_pResource; //リソース
	bool Load(std::string& path);
	bool Load(std::wstring& path);

	static ID3D12Resource* GetDefaultResource(size_t width, size_t height);

	Texture2D(const Texture2D&) = delete;
	void operator = (const Texture2D&) = delete;
};

static std::unordered_map<std::wstring, Texture2D*> textures;