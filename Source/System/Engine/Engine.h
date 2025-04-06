#pragma once

#include <dxgi1_6.h>
#include <vector>
#include <filesystem>

#include "Lights\ZShadow.h"
#include "Input\\Input.h"
#include "SoundSystem\\BassSoundSystem.h"
#include "SoundSystem\\WwiseSoundSystem.h"
#include "SkyBox\\SkyBox.h"
#include "Effects\\EffectManager.h"
#include "UI\\UIManager.h"

#include "Model\\Animation\\AnimationManager.h"
#include "Model\\ModelManager.h"
#include "Model\\Character.h"

#include "Core\\Exception\\DxSystemException.h"
#include "Core\\ResourceCopy\\ResourceCopy.h"
#include "Core\\PostProcess\\PostProcessManager.h"

#include <pix3.h>

class Character;

class Engine
{
public:
	Engine(HWND hwnd);
	~Engine();

	//エンジン初期化
	bool Init(int windowWidth, int windowHeight);

	//ファイルから3Dモデル(ボーン付き)を読み込む
	Character* AddCharacter(std::string modelFile);
	//ファイルから3Dモデルを読み込む
	Model* AddModel(std::string modelFile);
	//クアッドモデルを読み込む
	Model* AddPrimitiveQuad();

	//モデルを解放
	void ReleaseModel(Model* pModel);

	//描画の開始処理
	void Begin3DRender();
	//3Dを描画
	void ModelRender();
	//2Dを描画
	void Begin2DRender();
	void ApplyPostProcess();
	//描画の終了処理
	void EndRender();

	//エンジンの更新
	void Update();
	void LateUpdate();

	void SetEnablePostProcess(bool value);

	void ResetViewportAndScissor();

	//キー操作をフォーカスがない状態でも受け付けるかどうかを設定
	void SetKeyResponseUnFocus(bool bCanResponse);

	//バックカラーを設定
	void SetClearColor(DXGI_RGBA clearColor);

	//ウィンドウのスタイルを変更
	//引数 : WindowMode モード, SIZE ウィンドウサイズ
	//※windowSizeはWindowMode = WindowNormalのみ指定
	void SetWindowMode(WindowMode windowMode, _In_opt_ SIZE* windowSize);

	//ファイルからアニメーションをロード
	//既にロード済み
	Animation* GetAnimation(std::string animFilePath);

public: //ゲッター関数
	//マウスの状態を取得
	BYTE GetMouseState(BYTE keyCode) const;
	//マウスのボタン状態を取得 (押した瞬間のみ)
	BYTE GetMouseStateSync(BYTE keyCode);
	//マウスのボタン状態を取得 (離した瞬間のみ)
	BYTE GetMouseStateRelease(BYTE keyCode);

	//マウスの移動量を取得
	POINT GetMouseMove() const;
	//マウス座標を取得
	POINT GetMousePosition() const;

	//キーの状態を取得 (押しっぱなし)
	bool GetKeyState(UINT key);
	//キーの状態を取得 (押した瞬間のみ)
	bool GetKeyStateSync(UINT key);

	inline HWND GetHWND() const { return m_hWnd; }

	//エンジンのデバイス
	inline ID3D12Device6* GetDevice() const { return m_pDevice.Get(); }

	//コマンドリスト
	inline ID3D12GraphicsCommandList4* GetCommandList() const { return m_pCommandList.Get(); }

	//Bass Audio Library - サウンドシステム
	inline BassSoundSystem* GetBassSoundSystem() { return &m_bassSoundSystem; }

	//Wwise - サウンドシステム
	inline WwiseSoundSystem* GetWwiseSoundSystem() { return &m_wwiseSoundSystem; }

	//ディレクショナルライト
	inline DirectionalLight* GetDirectionalLight() { return &m_directionalLight; }

	//カメラを取得
	inline Camera* GetCamera() { return &m_camera; }

	//マテリアルシステム
	inline MaterialManager* GetMaterialManager() { return &m_materialManager; }

	//エフェクトシステム
	inline EffectManager* GetEffectManager() { return &m_effectManager; }

	//スカイボックス
	inline SkyBox* GetSkyBox() { return &m_skyBox; }

	//影
	inline ZShadow* GetZShadow() { return &m_zShadow; }

	//UIシステム
	inline UIManager* GetUIManager() { return &m_uiManager; }

	//トリプルバッファリングの現在のインデックス
	inline UINT CurrentBackBufferIndex() const { return m_CurrentBackBufferIndex; }

	//ポストプロセスが有効かどうか
	inline bool GetIsPostProcessEnabled() const { return m_bEnablePostProcess; }

	//前回のフレームから何秒経過したかを取得
	inline float GetFrameTime() const { return m_frameTime; }

	//背景色を取得
	inline DXGI_RGBA GetClearColor() const { return m_clearColor; }

	//現在のウィンドウモードを取得
	inline WindowMode GetWindowMode() const { return m_windowMode; }

	//現在のウィンドウサイズを取得
	inline SIZE GetWindowSize() const { return m_windowSize; }

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

	//描画完了を待つ処理
	void WaitRender();

private: //描画に使うDirectX12のオブジェクト
	HWND m_hWnd;
	UINT m_CurrentBackBufferIndex = 0;

	ComPtr<ID3D12Device6> m_pDevice = nullptr;		//デバイス
	ComPtr<ID3D12CommandQueue> m_pQueue = nullptr;	//コマンドキュー
	ComPtr<IDXGISwapChain3> m_pSwapChain = nullptr; //スワップチェイン
	ComPtr<IDXGIFactory6> m_pDxgiFactory = nullptr;	//ファクトリ(GPUの解析)
	ComPtr<ID3D12CommandAllocator> m_pAllocator = nullptr;	//コマンドアロケーター
	ComPtr<ID3D12GraphicsCommandList4> m_pCommandList = nullptr; //コマンドリスト
	ComPtr<ID3D12Debug1> m_pDebugController;			//デバッグコントローラー
	ComPtr<ID3D12Fence> m_pFence = nullptr;		//フェンス

	HANDLE m_fenceEvent = nullptr;				//フェンスで使うイベント
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
	ComPtr<ID3D12Resource> m_intermediateRenderTargets[FRAME_BUFFER_COUNT] = { nullptr };

	//深度ステンシルのディスクリプターサイズ
	UINT m_DsvDescriptorSize = 0;
	//深度ステンシルのディスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> m_pDsvHeap = nullptr;
	ComPtr<ID3D12Resource> m_pDepthStencilBuffer = nullptr;

	D3D12_RESOURCE_STATES m_beforeResourceState[FRAME_BUFFER_COUNT];

	AnimationManager m_animManager;
	EffectManager m_effectManager;
	MaterialManager m_materialManager;
	ModelManager m_modelManager;
	SkyBox m_skyBox;
	BassSoundSystem m_bassSoundSystem;
	WwiseSoundSystem m_wwiseSoundSystem;
	UIManager m_uiManager;
	ZShadow m_zShadow;

	PostProcessManager m_postProcessManager;

private: //プライベート変数
	unsigned long m_initTime;
	unsigned long m_sceneTimeMS;
	float m_frameTime;
	bool m_bEnablePostProcess;
	bool m_bResizeBuffer;

	SIZE m_windowSize;
	DXGI_RGBA m_clearColor;
	WindowMode m_windowMode;

	DirectionalLight m_directionalLight;

	Camera m_camera;
};

//どこからでも参照するためグローバル変数
extern Engine* g_Engine;
