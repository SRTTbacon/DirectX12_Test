#pragma once

#include <d3d12.h>

// "安全な"COMポインタの解放処理マクロ
#define SAFE_RELEASE(p) if(p){ p->Release(); p=NULL; }

// "安全な"配列の解放処理マクロ
#define SAFE_DELETE(p) if(p){ delete[] p; p=NULL; }

using namespace DirectX;

// 画像のクラス
class Texture2D
{
	// 状態
	enum
	{
		STATE_INIT = 0,
		STATE_COPY,
		STATE_COPY_WAIT,
		STATE_IDLE,
	};

public:
	Texture2D();
	~Texture2D();

	// 更新
	void Update(ID3D12GraphicsCommandList* pCommandList);

	// テクスチャーの作成
	bool CreateTexture(ID3D12Device* pDevice, const wchar_t* pFile);
	// シェーダーリソースビューの作成
	void CreateSRV(ID3D12Device* pDevice, D3D12_CPU_DESCRIPTOR_HANDLE handle);

	// コピー済みか
	bool IsCopied() const { return m_State > STATE_INIT; }


private:

	// テクスチャ情報をコピー
	void CopyTexture(ID3D12GraphicsCommandList* pCommandList);

	void CreateResource(ID3D12Device* pDevice, const Image* pImage, TexMetadata& metadata);


private:
	int		m_Count;
	int		m_State;

	// リソース情報
	size_t	m_RowPitch;

	ID3D12Resource* m_pTexture;
	ID3D12Resource* m_pTextureUpload;
};