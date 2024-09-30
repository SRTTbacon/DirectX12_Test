#pragma once
#include "..\\..\\..\\ComPtr.h"
#include <string>
#include <d3dx12.h>

class DescriptorHeap;
class DescriptorHandle;

class Texture3D
{
public:
	static Texture3D* Get(std::string path); // stringで受け取ったパスからテクスチャを読み込む
	static Texture3D* Get(std::wstring path); // wstringで受け取ったパスからテクスチャを読み込む
	static Texture3D* GetWhite(); // 白の単色テクスチャを生成する
	bool IsValid() const; // 正常に読み込まれているかどうかを返す

	ID3D12Resource* Resource(); // リソースを返す
	D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc(); // シェーダーリソースビューの設定を返す

private:
	bool m_IsValid; // 正常に読み込まれているか
	Texture3D(std::string path);
	Texture3D(std::wstring path);
	Texture3D(ID3D12Resource* buffer);
	ComPtr<ID3D12Resource> m_pResource; // リソース
	bool Load(std::string& path);
	bool Load(std::wstring& path);

	static ID3D12Resource* GetDefaultResource(size_t width, size_t height);

	Texture3D(const Texture3D&) = delete;
	void operator = (const Texture3D&) = delete;
};