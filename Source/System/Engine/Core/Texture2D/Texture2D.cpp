#include "Texture2D.h"
#include <DirectXTex.h>
#include "..\\..\\Engine.h"

#pragma comment(lib, "DirectXTex.lib")

using namespace DirectX;

static std::vector<TextureManage*> textures;

//拡張子を返す
static std::wstring FileExtension(const std::wstring& path)
{
	auto idx = path.rfind(L'.');
	return path.substr(idx + 1, path.length() - idx - 1);
}

Texture2D::Texture2D(std::string path)
{
	Load(path);
}

Texture2D::Texture2D(std::wstring path)
{
	Load(path);
}

Texture2D::Texture2D(TextureManage* pTextureManage)
{
	pTextureManage->refCount++;
	m_pManage = pTextureManage;
}

Texture2D::~Texture2D()
{
	if (m_pManage)
	{
		for (auto it = textures.begin(); it != textures.end(); it++)
		{
			if ((*it)->uniqueID == m_pManage->uniqueID)
			{
				(*it)->refCount--;
				if ((*it)->refCount <= 0)
				{
					(*it)->pResource.Reset();
					delete (*it);
					textures.erase(it);
				}
				break;
			}
		}
	}

	m_pManage = nullptr;
}

bool Texture2D::Load(std::string& path)
{
	auto wpath = GetWideString(path);
	return Load(wpath);
}

bool Texture2D::Load(std::wstring& path)
{
	UINT uniqueID = GenerateIDFromFile(path);

	//既に読み込まれているテクスチャかどうかを調べる
	for (TextureManage* tex : textures)
	{
		if (tex->uniqueID == uniqueID)
		{
			//既に読み込まれているテクスチャを参照
			m_pManage = tex;
			tex->refCount++;
			return true;
		}
	}

	//WICテクスチャのロード
	TexMetadata meta = {};
	ScratchImage scratch = {};

	HRESULT hr = LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, &meta, scratch);

	if (FAILED(hr))
	{
		printf("テクスチャの読み込みに失敗\n");
		return false;
	}

	//テクスチャ管理配列に登録
	TextureManage* manage = new TextureManage();
	manage->uniqueID = uniqueID;
	manage->refCount = 1;
	manage->bSimpleTex = false;

	const Image* img = scratch.GetImage(0, 0, 0);
	CD3DX12_HEAP_PROPERTIES prop = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(meta.format,
		static_cast<UINT>(meta.width),
		static_cast<UINT>(meta.height),
		static_cast<UINT16>(meta.arraySize),
		static_cast<UINT16>(meta.mipLevels));

	//リソースを生成
	hr = g_Engine->GetDevice()->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(manage->pResource.ReleaseAndGetAddressOf())
	);

	if (FAILED(hr))
	{
		printf("テクスチャの読み込みに失敗 エラーコード = %1x\n", hr);
		return false;
	}

	hr = manage->pResource->WriteToSubresource(0,
		nullptr,							//全領域へコピー
		img->pixels,						//元データアドレス
		static_cast<UINT>(img->rowPitch),	//1ラインサイズ
		static_cast<UINT>(img->slicePitch)	//全サイズ
	);
	if (FAILED(hr))
	{
		printf("テクスチャの読み込みに失敗\n");
		return false;
	}

	textures.push_back(manage);

	m_pManage = manage;

	return true;
}

Texture2D* Texture2D::Get(std::string path)
{
	auto wpath = GetWideString(path);
	return Get(wpath);
}

Texture2D* Texture2D::Get(std::wstring path)
{
	Texture2D* tex = new Texture2D(path);
	if (!tex->IsValid())
	{
		delete tex;
		return GetColor(0.0f, 0.0f, 0.0f); //読み込みに失敗した時は黒単色テクスチャを返す
	}
	return tex;
}

Texture2D* Texture2D::GetColor(float r, float g, float b)
{
	UINT width = 16;
	UINT height = 16;

	std::vector<UINT> data(width * height);
	std::fill(data.begin(), data.end(), ColorToUINT(r, g, b));

	//UINTデータをバイト単位に変換するためのstd::vector<char>
	std::vector<char> charVector(data.size() * sizeof(UINT));
	//データをコピー
	std::memcpy(charVector.data(), data.data(), data.size() * sizeof(UINT));

	//既に読み込まれているテクスチャかどうかを調べる
	UINT uniqueID = GenerateIDFromFile(charVector);
	for (TextureManage* tex : textures)
	{
		if (tex->uniqueID == uniqueID)
		{
			return new Texture2D(tex);
		}
	}

	ID3D12Resource* pBuffer = GetDefaultResource(DXGI_FORMAT_R8G8B8A8_UNORM, true, width, height);

	HRESULT hr = pBuffer->WriteToSubresource(0, nullptr, data.data(), width * sizeof(UINT), static_cast<UINT>(data.size() * sizeof(UINT)));
	if (FAILED(hr))
	{
		return nullptr;
	}

	TextureManage* manage = new TextureManage();
	manage->uniqueID = uniqueID;
	manage->refCount = 0;		//new Texture2D()で参照カウントが増えるので0
	manage->pResource = pBuffer;
	manage->bSimpleTex = true;

	textures.push_back(manage);

	return new Texture2D(manage);
}

ID3D12Resource* Texture2D::GetDefaultResource(DXGI_FORMAT format, bool bPixelShader, size_t width, size_t height)
{
	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, static_cast<UINT>(width), static_cast<UINT>(height), 1, 1);
	CD3DX12_HEAP_PROPERTIES texHeapProp = bPixelShader ? CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0) : CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ID3D12Resource* buff = nullptr;
	D3D12_RESOURCE_STATES state = bPixelShader ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_COPY_DEST;
	HRESULT result = g_Engine->GetDevice()->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		state,
		nullptr,
		IID_PPV_ARGS(&buff)
	);
	if (FAILED(result))
	{
		//assert(SUCCEEDED(result));
		printf("GetDefaultResource : エラーコード = %1x\n", result);
		return nullptr;
	}
	return buff;
}

bool Texture2D::IsValid() const
{
	return m_pManage != nullptr;
}

bool Texture2D::IsSimpleTex() const
{
	return m_pManage->bSimpleTex;
}

ID3D12Resource* Texture2D::Resource()
{
	if (!m_pManage) {
		return nullptr;
	}

	return m_pManage->pResource.Get();
}

D3D12_SHADER_RESOURCE_VIEW_DESC Texture2D::ViewDesc()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	if (m_pManage && m_pManage->pResource) {
		desc.Format = m_pManage->pResource->GetDesc().Format;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; //2Dテクスチャ
		desc.Texture2D.MipLevels = 1; //ミップマップは使用しないので1
	}
	return desc;
}
