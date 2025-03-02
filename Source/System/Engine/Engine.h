#pragma once

#include <dxgi1_6.h>
#include <vector>
#include <filesystem>

#include "Lights\ZShadow.h"

#include "Input\\Input.h"

#include "SoundSystem\\SoundSystem.h"

#include "Model\\Animation\\AnimationManager.h"
#include "Model\\ModelManager.h"
#include "Model\\Character.h"

#include <pix3.h>

class Character;

class Engine
{
public:
	Engine(HWND hwnd);
	~Engine();

	void Release();

	//エンジン初期化
	bool Init(UINT windowWidth, UINT windowHeight);

	Character* AddCharacter(std::string modelFile);
	Model* AddModel(std::string modelFile);

	//描画の開始処理
	void BeginRender();
	void ModelRender();
	//描画の終了処理
	void EndRender();

	//エンジンの更新
	void Update();
	void LateUpdate();

	void ResetViewportAndScissor();

	//キー操作をフォーカスがない状態でも受け付けるかどうかを設定
	void SetKeyResponseUnFocus(bool bCanResponse);

	//ファイルからアニメーションをロード
	//既にロード済み
	Animation GetAnimation(std::string animFilePath);

public: //ゲッター関数
	//マウスの状態を取得
	BYTE GetMouseState(BYTE keyCode);
	//マウスのボタン状態を取得 (押した瞬間のみ)
	BYTE GetMouseStateSync(BYTE keyCode);
	//マウスの移動量を取得
	POINT GetMouseMove();
	//キーの状態を取得
	bool GetKeyState(UINT key);
	bool GetKeyStateSync(UINT key);

	//エンジンのデバイス
	inline ID3D12Device6* GetDevice()
	{
		return m_pDevice.Get();
	}

	//コマンドリスト
	inline ID3D12GraphicsCommandList* GetCommandList()
	{
		return m_pCommandList.Get();
	}

	//サウンドシステム
	inline SoundSystem* GetSoundSystem()
	{
		return &m_soundSystem;
	}

	//ディレクショナルライト
	inline DirectionalLight* GetDirectionalLight()
	{
		return &m_directionalLight;
	}

	//カメラを取得
	inline Camera* GetCamera()
	{
		return &m_camera;
	}

	//トリプルバッファリングの現在のインデックス
	inline UINT CurrentBackBufferIndex() const
	{
		return m_CurrentBackBufferIndex;
	}

	//前回のフレームから何秒経過したかを取得
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
	ComPtr<IDXGIFactory6> m_pDxgiFactory = nullptr;	//ファクトリ(GPUの解析)
	ComPtr<ID3D12CommandAllocator> m_pAllocator = nullptr;	//コマンドアロケーター
	ComPtr<ID3D12GraphicsCommandList4> m_pCommandList = nullptr; //コマンドリスト
	ComPtr<ID3D12Debug1> m_pDebugController;			//デバッグコントローラー

	HANDLE m_fenceEvent = nullptr;				//フェンスで使うイベント
	ComPtr<ID3D12Fence> m_pFence = nullptr;		//フェンス
	UINT64 m_fenceValue[FRAME_BUFFER_COUNT];	//フェンスの値（トリプルバッファリング用に3個）
	D3D12_VIEWPORT m_Viewport;					//ビューポート
	D3D12_RECT m_Scissor;						//シザー矩形
	Input m_keyInput;							//キー状態

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
	ComPtr<ID3D12Resource> m_pDepthStencilBuffer = nullptr;

	ModelManager m_modelManager;
	MaterialManager m_materialManager;
	ZShadow m_zShadow;

private: //描画ループで使用するもの
	//描画完了を待つ処理
	void WaitRender();

private: //プライベート変数
	unsigned long m_initTime;
	unsigned long m_sceneTimeMS;
	float m_frameTime;

	AnimationManager animManager;

	SoundSystem m_soundSystem;

	DirectionalLight m_directionalLight;

	Camera m_camera;
};

//どこからでも参照するためグローバル変数
extern Engine* g_Engine;
