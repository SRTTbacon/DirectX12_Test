#pragma once
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <d3dx12.h>
#include <vector>
#include "..\\ComPtr.h"
#include "Input\\Input.h"
#include <DirectXMath.h>
#include <timeapi.h>
#include <filesystem>

#include "..\\Main\\Utility.h"

#include "Lights\\DirectionalLight.h"

#include "SoundSystem\\SoundSystem.h"
#include "Model\\Animation\\AnimationManager.h"

#pragma comment(lib, "d3d12.lib") // d3d12ライブラリをリンクする
#pragma comment(lib, "dxgi.lib") // dxgiライブラリをリンクする

constexpr int FRAME_BUFFER_COUNT = 3;

class Engine
{
public:
	~Engine();

	//エンジン初期化
	bool Init(HWND hwnd, UINT windowWidth, UINT windowHeight);

	//描画の開始処理
	void BeginRender();
	//描画の終了処理
	void EndRender();

	//エンジンの更新
	void Update();

	//ファイルからアニメーションをロード
	//既にロード済み
	Animation GetAnimation(std::string animFilePath);

public: //ゲッター関数
	//マウスの状態を取得
	BYTE GetMouseState(BYTE keyCode);
	//マウスのボタン状態を取得 (押した瞬間のみ)
	BYTE GetMouseStateSync(BYTE keyCode);
	//キーの状態を取得
	bool GetKeyState(UINT key);
	bool GetKeyStateSync(UINT key);

	//エンジンのデバイス
	inline ID3D12Device6* Device()
	{
		return m_pDevice.Get();
	}

	//コマンドリスト
	inline ID3D12GraphicsCommandList* CommandList()
	{
		return m_pCommandList.Get();
	}

	//サウンドシステム
	inline SoundSystem* GetSoundSystem()
	{
		return m_pSoundSystem;
	}

	//ディレクショナルライト
	inline DirectionalLight* GetDirectionalLight()
	{
		return m_pDirectionalLight;
	}

	//トリプルバッファリングの現在のインデックス
	inline UINT CurrentBackBufferIndex() const
	{
		return m_CurrentBackBufferIndex;
	}

	//前回のフレームから何ミリ秒経過したかを取得
	inline float GetFrameTime() const
	{
		return m_frameTime;
	}

private: //DirectX12の初期化
	//デバイスを作成
	bool CreateDevice();
	//コマンドキューを生成
	bool CreateCommandQueue();
	//スワップチェインを生成 (トリプルバッファリング : 画面のちらつきを抑える)
	bool CreateSwapChain();
	//コマンドリストとコマンドアロケーターを生成
	bool CreateCommandList();
	//フェンスを生成
	bool CreateFence();
	//ビューポートを生成
	void CreateViewPort();
	//シザー矩形を生成
	void CreateScissorRect();

private: //描画に使うDirectX12のオブジェクト
	HWND m_hWnd;
	UINT m_FrameBufferWidth = 0;
	UINT m_FrameBufferHeight = 0;
	UINT m_CurrentBackBufferIndex = 0;

	ComPtr<ID3D12Device6> m_pDevice = nullptr;		//デバイス
	ComPtr<ID3D12CommandQueue> m_pQueue = nullptr;	//コマンドキュー
	ComPtr<IDXGISwapChain3> m_pSwapChain = nullptr; //スワップチェイン
	ComPtr<ID3D12CommandAllocator> m_pAllocator[FRAME_BUFFER_COUNT] = { nullptr };	//コマンドアロケーター
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList = nullptr;						//コマンドリスト
	HANDLE m_fenceEvent = nullptr;				//フェンスで使うイベント
	ComPtr<ID3D12Fence> m_pFence = nullptr;		//フェンス
	UINT64 m_fenceValue[FRAME_BUFFER_COUNT];	//フェンスの値（トリプルバッファリング用に3個）
	D3D12_VIEWPORT m_Viewport;					//ビューポート
	D3D12_RECT m_Scissor;						//シザー矩形
	Input* m_pKeyInput;							//キー状態

private: //描画に使うオブジェクト
	//レンダーターゲットを生成
	bool CreateRenderTarget();
	//深度ステンシルバッファを生成
	bool CreateDepthStencil();

	//レンダーターゲットビューのディスクリプタサイズ
	UINT m_RtvDescriptorSize = 0;
	//レンダーターゲットのディスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> m_pRtvHeap = nullptr;
	//レンダーターゲット（トリプルバッファリング用に3個）
	ComPtr<ID3D12Resource> m_pRenderTargets[FRAME_BUFFER_COUNT] = { nullptr };

	//深度ステンシルのディスクリプターサイズ
	UINT m_DsvDescriptorSize = 0;
	//深度ステンシルのディスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> m_pDsvHeap = nullptr;
	//深度ステンシルバッファ
	ComPtr<ID3D12Resource> m_pDepthStencilBuffer = nullptr;

private: //描画ループで使用するもの
	//現在のフレームのレンダーターゲットを一時的に保存
	ID3D12Resource* m_currentRenderTarget = nullptr;

	//描画完了を待つ処理
	void WaitRender();

private: //プライベート変数
	unsigned long m_initTime;
	unsigned long m_sceneTimeMS;
	float m_frameTime;

	AnimationManager animManager;

	SoundSystem* m_pSoundSystem;

	DirectionalLight* m_pDirectionalLight;
};

//どこからでも参照するためグローバル変数
extern Engine* g_Engine;
