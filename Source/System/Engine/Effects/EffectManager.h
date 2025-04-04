#pragma once

#include <EffekseerRendererDX12.h>

#include "..\\Camera\\Camera.h"
#include "..\\..\\Main\\Utility.h"
#include "Effect.h"

//Effekseerを使用したエフェクトの描画

class EffectManager
{
public:
	friend class Engine;

	EffectManager();
	~EffectManager();

	//ファイルからエフェクトをロード
	//@return 読み込んだエフェクトを制御するクラス
	Effect* CreateEffect(std::string path);
	//メモリからエフェクトをロード
	Effect* CreateEffect(const void* data, int size);

private:
	Effekseer::ManagerRef m_effekseerManager;
	EffekseerRenderer::RendererRef m_effekseerRenderer;
	Effekseer::RefPtr<EffekseerRenderer::SingleFrameMemoryPool> m_efkMemoryPool;
	Effekseer::RefPtr<EffekseerRenderer::CommandList> m_efkCommandList;

	ID3D12GraphicsCommandList* m_pCommandList;
	Camera* m_pCamera;

	std::vector<std::unique_ptr<Effect>> m_effects;

	//初期化
	//@param ID3D12Device* : エンジンのデバイス
	//@param ID3D12CommandQueue* : エンジンのコマンドキュー
	//@param ID3D12GraphicsCommandList* : エンジンのコマンドリスト
	//@param Camera* : エンジンのカメラクラス
	//@param UINT : バッファの数 (基本的にはエンジンのバッファ数と同じ値を入れる)
	//@param int : エフェクトの最大描画数 (インスタンス単位。1つのエフェクトに何個もインスタンスが存在する場合がある)
	void Initialize(ID3D12Device* pDevice, ID3D12CommandQueue* pCommandQueue, ID3D12GraphicsCommandList* pCommandList, Camera* pCamera, UINT bufferCount, int maxEffectCount);

	//エフェクトの更新
	//エンジン側で実行される
	void LateUpdate();

	//エフェクトのレンダリング開始
	//エンジン側で実行される
	void BeginRender();
	//エフェクトのレンダリング終了
	//エンジン側で実行される
	void EndRender();

	//エフェクトを描画
	//エンジン側で実行される
	void Draw();

	//XMMATRIXをEffekseerで使用するMatrix44に変換
	Effekseer::Matrix44 ConvertMatrix(DirectX::XMMATRIX& mat);

	Effect* AddEffect(Effekseer::EffectRef effect);
};
