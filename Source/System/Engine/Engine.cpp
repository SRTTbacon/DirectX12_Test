#include "Engine.h"

Engine* g_Engine = nullptr;

Engine::Engine(HWND hwnd)
	: m_Scissor(D3D12_RECT())
	, m_Viewport(D3D12_VIEWPORT())
	, m_clearColor(DXGI_RGBA(0.0f, 0.0f, 0.0f, 1.0f))
	, m_fenceValue{0}
	, m_frameTime(0.0f)
	, m_hWnd(hwnd)
	, m_initTime(0)
	, m_keyInput(hwnd)
	, m_bassSoundSystem(hwnd)
	, m_sceneTimeMS(0)
	, m_windowSize(0, 0)
	, m_bEnablePostProcess(false)
	, m_bResizeBuffer(false)
{
}

Engine::~Engine()
{
	m_wwiseSoundSystem.Dispose();
}

bool Engine::Init(int windowWidth, int windowHeight)
{
	m_windowSize = SIZE(windowWidth, windowHeight);

#ifdef _DEBUG
	//デバッグレイヤーの設定
	
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_pDebugController)))) {
		m_pDebugController->SetEnableGPUBasedValidation(TRUE);
		m_pDebugController->SetEnableSynchronizedCommandQueueValidation(TRUE);
		m_pDebugController->EnableDebugLayer();
		printf("デバッグレイヤーを有効化しました。\n");
	}
#endif

	//ビューポートとシザー矩形を生成
	CreateViewPort();
	CreateScissorRect();

	if (!CreateDevice())
	{
		printf("デバイスの生成に失敗しました。GPUがレイトレーシングに対応していない可能性があります。\n");
		return false;
	}
	if (!CreateCommandQueue())
	{
		printf("コマンドキューの生成に失敗\n");
		return false;
	}

	if (!CreateSwapChain())
	{
		printf("スワップチェインの生成に失敗\n");
		return false;
	}

	if (!CreateCommandList())
	{
		printf("コマンドリストの生成に失敗\n");
		return false;
	}

	if (!CreateFence())
	{
		printf("フェンスの生成に失敗\n");
		return false;
	}

	if (!CreateRenderTarget())
	{
		printf("レンダーターゲットの生成に失敗\n");
		return false;
	}

	if (!CreateDepthStencil())
	{
		printf("デプスステンシルバッファの生成に失敗\n");
		return false;
	}

	g_resourceCopy->Initialize(m_pDevice.Get());

	//FrameTime計測用
	m_initTime = timeGetTime();
	m_sceneTimeMS = 0;
	m_frameTime = 0.0f;

	//ディレクショナルライトの初期化
	m_directionalLight.Init(m_pDevice.Get());

	//影を初期化
	m_zShadow.Init(m_pDevice.Get(), m_pCommandList.Get());

	//マテリアル管理クラスを初期化
	m_materialManager.Initialize(m_pDevice.Get(), m_pCommandList.Get(), &m_directionalLight, m_zShadow.GetZBuffer());

	//エフェクト管理クラスを初期化
	m_effectManager.Initialize(m_pDevice.Get(), m_pQueue.Get(), m_pCommandList.Get(), & m_camera, FRAME_BUFFER_COUNT, 8000);

	//スカイボックスを初期化
	m_skyBox.Initialize(m_pDevice.Get(), m_pCommandList.Get(), &m_camera, &m_materialManager);

	//UI管理クラスを初期化
	m_uiManager.Initialize(m_pDevice.Get(), m_pCommandList.Get(), &m_windowSize);

	//ポストプロセスを初期化
	m_postProcessManager.Initialize(m_pDevice.Get(), m_pCommandList.Get());

	//コマンドを初期化してためる準備をする
	m_pAllocator->Reset();
	m_pCommandList->Reset(m_pAllocator.Get(), nullptr);

	srand(static_cast<UINT>(timeGetTime()));

	printf("描画エンジンの初期化に成功\n");
	return true;
}

Character* Engine::AddCharacter(std::string modelFile)
{
	Character* pCharacter = new Character(modelFile, m_pDevice.Get(), m_pCommandList.Get(), &m_camera, &m_directionalLight, &m_materialManager, &m_frameTime);
	m_modelManager.AddModel(pCharacter);
	return pCharacter;
}

Model* Engine::AddModel(std::string modelFile)
{
	Model* pModel = new Model(m_pDevice.Get(), m_pCommandList.Get(), &m_camera, &m_directionalLight, &m_materialManager, &m_frameTime);
	pModel->LoadModel(modelFile);
	m_modelManager.AddModel(pModel);
	return pModel;
}

Model* Engine::AddPrimitiveQuad()
{
	Model* pModel = new Model(m_pDevice.Get(), m_pCommandList.Get(), &m_camera, &m_directionalLight, &m_materialManager, &m_frameTime);
	pModel->LoadPrimitiveQuad();
	m_modelManager.AddModel(pModel);
	return pModel;
}

void Engine::ReleaseModel(Model* pModel)
{
	m_modelManager.ReleaseModel(pModel);
}

bool Engine::CreateDevice()
{
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&m_pDxgiFactory));

	if (FAILED(hr)) {
		return false;
	}

	//レイトレーシングをサポートするGPUを取得
	ComPtr<IDXGIAdapter1> adapter;
	for (UINT i = 0; ; ++i) {
		hr = m_pDxgiFactory->EnumAdapters1(i, &adapter);
		if (FAILED(hr)) {
			break;
		}

		// レイトレーシング機能を持つアダプタか確認
		hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_pDevice));

		if (SUCCEEDED(hr)) {
			// レイトレーシングをサポートしているか確認
			D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5{};
			hr = m_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));

			if (SUCCEEDED(hr) && options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED) {
				return true;
			}
		}
	}
	return false;
}

bool Engine::CreateCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 1;

	auto hr = m_pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(m_pQueue.ReleaseAndGetAddressOf()));

	return SUCCEEDED(hr);
}

bool Engine::CreateSwapChain()
{
	//DXGIファクトリーの生成
	IDXGIFactory4* pFactory = nullptr;
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&pFactory));
	if (FAILED(hr))
	{
		return false;
	}

	//スワップチェインの生成
	DXGI_SWAP_CHAIN_DESC desc = {};
	desc.BufferDesc.Width = static_cast<UINT>(m_windowSize.cx);
	desc.BufferDesc.Height = static_cast<UINT>(m_windowSize.cy);
	desc.BufferDesc.RefreshRate.Numerator = 60;
	desc.BufferDesc.RefreshRate.Denominator = 1;
	desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = FRAME_BUFFER_COUNT;
	desc.OutputWindow = m_hWnd;
	desc.Windowed = TRUE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	//スワップチェインの生成
	IDXGISwapChain* pSwapChain = nullptr;
	hr = pFactory->CreateSwapChain(m_pQueue.Get(), &desc, &pSwapChain);
	if (FAILED(hr))
	{
		pFactory->Release();
		return false;
	}

	//IDXGISwapChain3を取得
	hr = pSwapChain->QueryInterface(IID_PPV_ARGS(m_pSwapChain.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		pFactory->Release();
		pSwapChain->Release();
		return false;
	}

	//バックバッファ番号を取得
	m_CurrentBackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	pFactory->Release();
	pSwapChain->Release();
	return true;
}

bool Engine::CreateCommandList()
{
	//コマンドアロケーターの作成
	HRESULT hr = m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_pAllocator.ReleaseAndGetAddressOf()));

	if (FAILED(hr)) {
		return false;
	}

	//標準のグラフィックスコマンドリストを作成
	hr = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pAllocator.Get(), nullptr, IID_PPV_ARGS(&m_pCommandList));
	if (FAILED(hr)) {
		return false;
	}

	//コマンドリストは開かれている状態で作成されるので、いったん閉じる。
	m_pCommandList->Close();

	return true;
}

bool Engine::CreateFence()
{
	for (auto i = 0u; i < FRAME_BUFFER_COUNT; i++)
	{
		m_fenceValue[i] = 0;
	}

	auto hr = m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		return false;
	}

	m_fenceValue[m_CurrentBackBufferIndex]++;

	//同期を行うときのイベントハンドラを作成する。
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	return m_fenceEvent != nullptr;
}

void Engine::CreateViewPort()
{
	m_Viewport.TopLeftX = 0;
	m_Viewport.TopLeftY = 0;
	m_Viewport.Width = static_cast<float>(m_windowSize.cx);
	m_Viewport.Height = static_cast<float>(m_windowSize.cy);
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;
}

void Engine::CreateScissorRect()
{
	m_Scissor.left = 0;
	m_Scissor.right = m_windowSize.cx;
	m_Scissor.top = 0;
	m_Scissor.bottom = m_windowSize.cy;
}

bool Engine::CreateRenderTarget()
{
	//RTV用のディスクリプタヒープを作成
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = FRAME_BUFFER_COUNT * 2; //フレームバッファ+中間テクスチャ
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	auto hr = m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_pRtvHeap.ReleaseAndGetAddressOf()));
	if (FAILED(hr)) {
		return false;
	}

	//ディスクリプタのサイズを取得
	m_RtvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();

	//スワップチェーンのバックバッファを取得
	for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++) {
		m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_pRenderTargets[i].ReleaseAndGetAddressOf()));
		m_pDevice->CreateRenderTargetView(m_pRenderTargets[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += m_RtvDescriptorSize;
	}

	//中間テクスチャをフレーム数分作成
	D3D12_RESOURCE_DESC desc2 = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, static_cast<UINT64>(m_Viewport.Width), static_cast<UINT>(m_Viewport.Height));
	desc2.SampleDesc.Count = 1;
	desc2.MipLevels = 1;
	desc2.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	clearValue.Color[0] = m_clearColor.r;
	clearValue.Color[1] = m_clearColor.g;
	clearValue.Color[2] = m_clearColor.b;
	clearValue.Color[3] = 1.0f; //アルファを1.0に設定

	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		hr = m_pDevice->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&desc2,
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			&clearValue,
			IID_PPV_ARGS(&m_intermediateRenderTargets[i])
		);
		if (FAILED(hr)) {
			return false;
		}

		//中間テクスチャ用のRTVを作成
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		m_pDevice->CreateRenderTargetView(m_intermediateRenderTargets[i].Get(), &rtvDesc, rtvHandle);
		rtvHandle.ptr += m_RtvDescriptorSize;

		m_beforeResourceState[i] = D3D12_RESOURCE_STATE_COPY_SOURCE;
	}

	return true;
}

bool Engine::CreateDepthStencil()
{
	//DSV用のディスクリプタヒープを作成する
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	auto hr = m_pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_pDsvHeap));
	if (FAILED(hr))
	{
		return false;
	}

	//ディスクリプタのサイズを取得
	m_DsvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	D3D12_CLEAR_VALUE dsvClearValue{};
	dsvClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	dsvClearValue.DepthStencil.Depth = 1.0f;
	dsvClearValue.DepthStencil.Stencil = 0;

	CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC resourceDesc(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		static_cast<UINT64>(m_windowSize.cx),
		static_cast<UINT>(m_windowSize.cy),
		1,
		1,
		DXGI_FORMAT_D32_FLOAT,
		1,
		0,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	hr = m_pDevice->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&dsvClearValue,
		IID_PPV_ARGS(m_pDepthStencilBuffer.ReleaseAndGetAddressOf())
	);

	if (FAILED(hr))
	{
		return false;
	}

	//ディスクリプタを作成
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_pDsvHeap->GetCPUDescriptorHandleForHeapStart();

	//深度ステンシルビュー設定用構造体の設定
	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
	desc.Format = DXGI_FORMAT_D32_FLOAT;
	desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	desc.Flags = D3D12_DSV_FLAG_NONE;

	m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer.Get(), &desc, dsvHandle);

	return true;
}

void Engine::Begin3DRender()
{
	//現在のレンダーターゲットを更新
	ID3D12Resource* pCurrentIntermediateTarget = m_intermediateRenderTargets[m_CurrentBackBufferIndex].Get();

	//ビューポートとシザー矩形を設定
	m_pCommandList->RSSetViewports(1, &m_Viewport);
	m_pCommandList->RSSetScissorRects(1, &m_Scissor);

	//レンダーターゲットが使用可能になるまで待つ
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pCurrentIntermediateTarget, m_beforeResourceState[m_CurrentBackBufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_pCommandList->ResourceBarrier(1, &barrier);
}

void Engine::ModelRender()
{
#if _DEBUG
	//PIXBeginEvent(m_pCommandList.Get(), PIX_COLOR(255, 0, 0), "Render Frame");
#endif
	m_materialManager.SetHeap();

	//モデルの影の描画
	m_zShadow.BeginMapping();
	m_modelManager.RenderShadowMap(m_CurrentBackBufferIndex);

	//深度マップを変更
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = m_zShadow.GetZBuffer();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_pCommandList->ResourceBarrier(1, &barrier);

	ResetViewportAndScissor();

	//スカイボックスを描画
	m_skyBox.Draw();
	
	//モデルを描画
	m_modelManager.RenderModel(m_pCommandList.Get(), m_CurrentBackBufferIndex);

	m_effectManager.BeginRender();
	m_effectManager.Draw();
	m_effectManager.EndRender();

	//深度マップを変更
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	m_pCommandList->ResourceBarrier(1, &barrier);
}

void Engine::Begin2DRender()
{
	m_uiManager.BeginRender();
}

void Engine::ApplyPostProcess()
{
	if (m_bEnablePostProcess) {
		// 中間テクスチャをシェーダーリソースとして使用
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_intermediateRenderTargets[m_CurrentBackBufferIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
		m_pCommandList->ResourceBarrier(1, &barrier);
		m_beforeResourceState[m_CurrentBackBufferIndex] = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

		//スワップチェーンのバックバッファを取得
		ID3D12Resource* pCurrentRenderTarget = m_pRenderTargets[m_CurrentBackBufferIndex].Get();

		//状態遷移：バックバッファをレンダーターゲットとして設定
		barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			pCurrentRenderTarget,
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		m_pCommandList->ResourceBarrier(1, &barrier);

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += static_cast<UINT64>(m_CurrentBackBufferIndex * m_RtvDescriptorSize);

		m_pCommandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

		m_postProcessManager.Render(m_intermediateRenderTargets[m_CurrentBackBufferIndex].Get());

		//状態遷移：バックバッファを表示状態に
		barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			pCurrentRenderTarget,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		);
		m_pCommandList->ResourceBarrier(1, &barrier);
	}
	else {
		// 中間テクスチャをシェーダーリソースとして使用
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_intermediateRenderTargets[m_CurrentBackBufferIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_COPY_SOURCE
		);
		m_pCommandList->ResourceBarrier(1, &barrier);
		m_beforeResourceState[m_CurrentBackBufferIndex] = D3D12_RESOURCE_STATE_COPY_SOURCE;

		//スワップチェーンのバックバッファを取得
		ID3D12Resource* pCurrentRenderTarget = m_pRenderTargets[m_CurrentBackBufferIndex].Get();

		//状態遷移：バックバッファをレンダーターゲットとして設定
		barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			pCurrentRenderTarget,
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_COPY_DEST
		);
		m_pCommandList->ResourceBarrier(1, &barrier);

		m_pCommandList->CopyResource(pCurrentRenderTarget, m_intermediateRenderTargets[m_CurrentBackBufferIndex].Get());

		//状態遷移：バックバッファを表示状態に
		barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			pCurrentRenderTarget,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PRESENT
		);
		m_pCommandList->ResourceBarrier(1, &barrier);
	}
}

void Engine::WaitRender()
{
	const UINT64 currentFanceValue = m_fenceValue[m_CurrentBackBufferIndex];
	m_pQueue->Signal(m_pFence.Get(), currentFanceValue);

	m_CurrentBackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// GPUが描画を終えていれば即座に次のフレームへ
	/*if (m_pFence->GetCompletedValue() >= currentFanceValue) {
		m_fenceValue[m_CurrentBackBufferIndex] = currentFanceValue + 1;
		return;
	}*/

	// GPUが未完了の場合は待機
	HRESULT hr = m_pFence->SetEventOnCompletion(currentFanceValue, m_fenceEvent);
	if (FAILED(hr)) {
		return;
	}
	WaitForSingleObject(m_fenceEvent, INFINITE);

	m_fenceValue[m_CurrentBackBufferIndex] = currentFanceValue + 1;

#if _DEBUG
	//PIXEndEvent(m_pCommandList.Get());
#endif
}

void Engine::EndRender()
{
	ApplyPostProcess();

	//コマンドの記録を終了
	m_pCommandList->Close();

	//コマンドを実行
	ID3D12CommandList* ppCmdLists[] = { m_pCommandList.Get() };
	m_pQueue->ExecuteCommandLists(1, ppCmdLists);

	//スワップチェーンを切り替え
	m_pSwapChain->Present(0, 0);

	//描画完了を待つ
	WaitRender();

	//コマンドを初期化してためる準備をする
	m_pAllocator->Reset();
	m_pCommandList->Reset(m_pAllocator.Get(), nullptr);

	if (m_bResizeBuffer) {
		for (UINT i = 0; i < FRAME_BUFFER_COUNT; ++i)
		{
			m_pRenderTargets[i].Reset();
			m_intermediateRenderTargets[i].Reset();
		}

		//スワップチェインのバッファサイズをリサイズ
		HRESULT hr = m_pSwapChain->ResizeBuffers(FRAME_BUFFER_COUNT, m_windowSize.cx, m_windowSize.cy, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
		if (FAILED(hr)) {
			printf("スワップチェインのリサイズに失敗しました。\n");
		}

		CreateViewPort();
		CreateRenderTarget();

		WaitRender();

		m_bResizeBuffer = false;
	}
}

BYTE Engine::GetMouseState(BYTE keyCode) const
{
	return m_keyInput.GetMouseState(keyCode);
}

BYTE Engine::GetMouseStateSync(BYTE keyCode)
{
	return m_keyInput.GetMouseStateSync(keyCode);
}

BYTE Engine::GetMouseStateRelease(BYTE keyCode)
{
	return m_keyInput.GetMouseStateRelease(keyCode);
}

POINT Engine::GetMouseMove() const
{
	return m_keyInput.GetMouseMove();
}

POINT Engine::GetMousePosition() const
{
	return m_keyInput.GetMousePosition();
}

bool Engine::GetKeyState(UINT key)
{
	return m_keyInput.CheckKey(key);
}

bool Engine::GetKeyStateSync(UINT key)
{
	return m_keyInput.TriggerKey(key);
}

void Engine::Update()
{
	//現在の時間
	DWORD timeNow = timeGetTime();
	//前のフレームから何ミリ秒経過したか
	DWORD nowFrameTimeMS = timeNow - m_sceneTimeMS - m_initTime;
	//フレーム時間を秒に直す
	m_frameTime = nowFrameTimeMS / 1000.0f;

	//フリーズ対策
	if (m_frameTime > 0.5f) {
		m_frameTime = 0.5f;
	}

	//シーンが切り替わってからの時間(ミリ秒と秒)
	m_sceneTimeMS = timeNow - m_initTime;

	//マウスの状態を更新
	m_keyInput.Update();

	//サウンドを更新
	m_bassSoundSystem.Update();

	//UIを更新
	m_uiManager.Update();
}

void Engine::LateUpdate()
{
	m_camera.LateUpdate(&m_directionalLight);

	m_directionalLight.LateUpdate();

	m_modelManager.LateUpdate(m_CurrentBackBufferIndex);

	m_skyBox.LateUpdate();

	m_effectManager.LateUpdate();
}

void Engine::SetEnablePostProcess(bool value)
{
	m_bEnablePostProcess = value;
}

void Engine::ResetViewportAndScissor()
{
	//ビューポートとシザー矩形を設定
	m_pCommandList->RSSetViewports(1, &m_Viewport);
	m_pCommandList->RSSetScissorRects(1, &m_Scissor);

	//中間テクスチャ用のRTVを設定
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += static_cast<UINT64>((m_CurrentBackBufferIndex + FRAME_BUFFER_COUNT) * m_RtvDescriptorSize);

	//深度ステンシルビューを取得
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_pDsvHeap->GetCPUDescriptorHandleForHeapStart();

	//RTVとDSVを設定
	m_pCommandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

	//深度ステンシルをクリア
	m_pCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//中間テクスチャをクリア
	const float clearColor[] = { m_clearColor.r, m_clearColor.g, m_clearColor.b, 1.0f };
	m_pCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}

void Engine::SetKeyResponseUnFocus(bool bCanResponse)
{
	m_keyInput.m_bCanResponseUnFocus = bCanResponse;
}

void Engine::SetClearColor(DXGI_RGBA clearColor)
{
	m_clearColor = clearColor;
}

void Engine::SetWindowMode(WindowMode windowMode, _In_opt_ SIZE* windowSize)
{
	if (windowMode == WindowMode::WindowNormal && !windowSize) {
		printf("WindowModeがWindowNormalの場合はwindowSizeを指定する必要があります。\n");
		return;
	}

	//ボーダーレスウィンドウに切り替え
	LONG style = 0;
	if (windowMode == WindowMode::WindowNormal || windowMode == WindowMode::WindowMaximum) {
		style |= WS_OVERLAPPEDWINDOW;
		style &= ~WS_SIZEBOX;
	}
	else {
		style = WS_OVERLAPPED;
	}

	SetWindowLong(m_hWnd, GWL_STYLE, style);

	//モニターのサイズを取得
	HMONITOR hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO monitorInfo = {};
	monitorInfo.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(hMonitor, &monitorInfo);

	LONG titleBarHeight = GetSystemMetrics(SM_CYCAPTION);

	LONG monitorWidth = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
	LONG monitorHeight = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;

	if (windowMode == WindowMode::WindowNormal) {
		m_windowSize.cx = std::min(windowSize->cx, monitorWidth);
		m_windowSize.cy = std::min(windowSize->cy, monitorHeight);
	}
	else if (windowMode == WindowMode::WindowMaximum) {
		m_windowSize.cx = monitorWidth;
		m_windowSize.cy = monitorHeight - titleBarHeight;
	}
	else {
		m_windowSize.cx = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
		m_windowSize.cy = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
	}

	if (windowMode == WindowMode::WindowNormal) {
		POINT centerPos{ 0, 0 };
		centerPos.x = monitorWidth / 4;
		centerPos.x = monitorHeight / 4;

		//ウィンドウの位置とサイズを調整
		SetWindowPos(m_hWnd, HWND_TOP, centerPos.x, centerPos.y, m_windowSize.cx, m_windowSize.cy, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);
		ShowWindow(m_hWnd, SW_SHOW);
	}
	else {
		SetWindowPos(m_hWnd, HWND_TOP, 0, 0, m_windowSize.cx, m_windowSize.cy, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
		ShowWindow(m_hWnd, SW_MAXIMIZE);
	}

	m_bResizeBuffer = true;
}

Animation* Engine::GetAnimation(std::string animFilePath)
{
	if (!std::filesystem::exists(animFilePath))
		return nullptr;

	return m_animManager.LoadAnimation(animFilePath);
}
