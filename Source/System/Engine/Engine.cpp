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
	//�f�o�b�O���C���[�̐ݒ�
	
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_pDebugController)))) {
		m_pDebugController->SetEnableGPUBasedValidation(TRUE);
		m_pDebugController->SetEnableSynchronizedCommandQueueValidation(TRUE);
		m_pDebugController->EnableDebugLayer();
		printf("�f�o�b�O���C���[��L�������܂����B\n");
	}
#endif

	//�r���[�|�[�g�ƃV�U�[��`�𐶐�
	CreateViewPort();
	CreateScissorRect();

	if (!CreateDevice())
	{
		printf("�f�o�C�X�̐����Ɏ��s���܂����BGPU�����C�g���[�V���O�ɑΉ����Ă��Ȃ��\��������܂��B\n");
		return false;
	}
	if (!CreateCommandQueue())
	{
		printf("�R�}���h�L���[�̐����Ɏ��s\n");
		return false;
	}

	if (!CreateSwapChain())
	{
		printf("�X���b�v�`�F�C���̐����Ɏ��s\n");
		return false;
	}

	if (!CreateCommandList())
	{
		printf("�R�}���h���X�g�̐����Ɏ��s\n");
		return false;
	}

	if (!CreateFence())
	{
		printf("�t�F���X�̐����Ɏ��s\n");
		return false;
	}

	if (!CreateRenderTarget())
	{
		printf("�����_�[�^�[�Q�b�g�̐����Ɏ��s\n");
		return false;
	}

	if (!CreateDepthStencil())
	{
		printf("�f�v�X�X�e���V���o�b�t�@�̐����Ɏ��s\n");
		return false;
	}

	g_resourceCopy->Initialize(m_pDevice.Get());

	//FrameTime�v���p
	m_initTime = timeGetTime();
	m_sceneTimeMS = 0;
	m_frameTime = 0.0f;

	//�f�B���N�V���i�����C�g�̏�����
	m_directionalLight.Init(m_pDevice.Get());

	//�e��������
	m_zShadow.Init(m_pDevice.Get(), m_pCommandList.Get());

	//�}�e���A���Ǘ��N���X��������
	m_materialManager.Initialize(m_pDevice.Get(), m_pCommandList.Get(), &m_directionalLight, m_zShadow.GetZBuffer());

	//�G�t�F�N�g�Ǘ��N���X��������
	m_effectManager.Initialize(m_pDevice.Get(), m_pQueue.Get(), m_pCommandList.Get(), & m_camera, FRAME_BUFFER_COUNT, 8000);

	//�X�J�C�{�b�N�X��������
	m_skyBox.Initialize(m_pDevice.Get(), m_pCommandList.Get(), &m_camera, &m_materialManager);

	//UI�Ǘ��N���X��������
	m_uiManager.Initialize(m_pDevice.Get(), m_pCommandList.Get(), &m_windowSize);

	//�|�X�g�v���Z�X��������
	m_postProcessManager.Initialize(m_pDevice.Get(), m_pCommandList.Get());

	//�R�}���h�����������Ă��߂鏀��������
	m_pAllocator->Reset();
	m_pCommandList->Reset(m_pAllocator.Get(), nullptr);

	srand(static_cast<UINT>(timeGetTime()));

	printf("�`��G���W���̏������ɐ���\n");
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

	//���C�g���[�V���O���T�|�[�g����GPU���擾
	ComPtr<IDXGIAdapter1> adapter;
	for (UINT i = 0; ; ++i) {
		hr = m_pDxgiFactory->EnumAdapters1(i, &adapter);
		if (FAILED(hr)) {
			break;
		}

		// ���C�g���[�V���O�@�\�����A�_�v�^���m�F
		hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_pDevice));

		if (SUCCEEDED(hr)) {
			// ���C�g���[�V���O���T�|�[�g���Ă��邩�m�F
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
	//DXGI�t�@�N�g���[�̐���
	IDXGIFactory4* pFactory = nullptr;
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&pFactory));
	if (FAILED(hr))
	{
		return false;
	}

	//�X���b�v�`�F�C���̐���
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

	//�X���b�v�`�F�C���̐���
	IDXGISwapChain* pSwapChain = nullptr;
	hr = pFactory->CreateSwapChain(m_pQueue.Get(), &desc, &pSwapChain);
	if (FAILED(hr))
	{
		pFactory->Release();
		return false;
	}

	//IDXGISwapChain3���擾
	hr = pSwapChain->QueryInterface(IID_PPV_ARGS(m_pSwapChain.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		pFactory->Release();
		pSwapChain->Release();
		return false;
	}

	//�o�b�N�o�b�t�@�ԍ����擾
	m_CurrentBackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	pFactory->Release();
	pSwapChain->Release();
	return true;
}

bool Engine::CreateCommandList()
{
	//�R�}���h�A���P�[�^�[�̍쐬
	HRESULT hr = m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_pAllocator.ReleaseAndGetAddressOf()));

	if (FAILED(hr)) {
		return false;
	}

	//�W���̃O���t�B�b�N�X�R�}���h���X�g���쐬
	hr = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pAllocator.Get(), nullptr, IID_PPV_ARGS(&m_pCommandList));
	if (FAILED(hr)) {
		return false;
	}

	//�R�}���h���X�g�͊J����Ă����Ԃō쐬�����̂ŁA�����������B
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

	//�������s���Ƃ��̃C�x���g�n���h�����쐬����B
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
	//RTV�p�̃f�B�X�N���v�^�q�[�v���쐬
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = FRAME_BUFFER_COUNT * 2; //�t���[���o�b�t�@+���ԃe�N�X�`��
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	auto hr = m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_pRtvHeap.ReleaseAndGetAddressOf()));
	if (FAILED(hr)) {
		return false;
	}

	//�f�B�X�N���v�^�̃T�C�Y���擾
	m_RtvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();

	//�X���b�v�`�F�[���̃o�b�N�o�b�t�@���擾
	for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++) {
		m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_pRenderTargets[i].ReleaseAndGetAddressOf()));
		m_pDevice->CreateRenderTargetView(m_pRenderTargets[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += m_RtvDescriptorSize;
	}

	//���ԃe�N�X�`�����t���[�������쐬
	D3D12_RESOURCE_DESC desc2 = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, static_cast<UINT64>(m_Viewport.Width), static_cast<UINT>(m_Viewport.Height));
	desc2.SampleDesc.Count = 1;
	desc2.MipLevels = 1;
	desc2.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	clearValue.Color[0] = m_clearColor.r;
	clearValue.Color[1] = m_clearColor.g;
	clearValue.Color[2] = m_clearColor.b;
	clearValue.Color[3] = 1.0f; //�A���t�@��1.0�ɐݒ�

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

		//���ԃe�N�X�`���p��RTV���쐬
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
	//DSV�p�̃f�B�X�N���v�^�q�[�v���쐬����
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	auto hr = m_pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_pDsvHeap));
	if (FAILED(hr))
	{
		return false;
	}

	//�f�B�X�N���v�^�̃T�C�Y���擾
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

	//�f�B�X�N���v�^���쐬
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_pDsvHeap->GetCPUDescriptorHandleForHeapStart();

	//�[�x�X�e���V���r���[�ݒ�p�\���̂̐ݒ�
	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
	desc.Format = DXGI_FORMAT_D32_FLOAT;
	desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	desc.Flags = D3D12_DSV_FLAG_NONE;

	m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer.Get(), &desc, dsvHandle);

	return true;
}

void Engine::Begin3DRender()
{
	//���݂̃����_�[�^�[�Q�b�g���X�V
	ID3D12Resource* pCurrentIntermediateTarget = m_intermediateRenderTargets[m_CurrentBackBufferIndex].Get();

	//�r���[�|�[�g�ƃV�U�[��`��ݒ�
	m_pCommandList->RSSetViewports(1, &m_Viewport);
	m_pCommandList->RSSetScissorRects(1, &m_Scissor);

	//�����_�[�^�[�Q�b�g���g�p�\�ɂȂ�܂ő҂�
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pCurrentIntermediateTarget, m_beforeResourceState[m_CurrentBackBufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_pCommandList->ResourceBarrier(1, &barrier);
}

void Engine::ModelRender()
{
#if _DEBUG
	//PIXBeginEvent(m_pCommandList.Get(), PIX_COLOR(255, 0, 0), "Render Frame");
#endif
	m_materialManager.SetHeap();

	//���f���̉e�̕`��
	m_zShadow.BeginMapping();
	m_modelManager.RenderShadowMap(m_CurrentBackBufferIndex);

	//�[�x�}�b�v��ύX
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = m_zShadow.GetZBuffer();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_pCommandList->ResourceBarrier(1, &barrier);

	ResetViewportAndScissor();

	//�X�J�C�{�b�N�X��`��
	m_skyBox.Draw();
	
	//���f����`��
	m_modelManager.RenderModel(m_pCommandList.Get(), m_CurrentBackBufferIndex);

	m_effectManager.BeginRender();
	m_effectManager.Draw();
	m_effectManager.EndRender();

	//�[�x�}�b�v��ύX
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
		// ���ԃe�N�X�`�����V�F�[�_�[���\�[�X�Ƃ��Ďg�p
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_intermediateRenderTargets[m_CurrentBackBufferIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
		m_pCommandList->ResourceBarrier(1, &barrier);
		m_beforeResourceState[m_CurrentBackBufferIndex] = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

		//�X���b�v�`�F�[���̃o�b�N�o�b�t�@���擾
		ID3D12Resource* pCurrentRenderTarget = m_pRenderTargets[m_CurrentBackBufferIndex].Get();

		//��ԑJ�ځF�o�b�N�o�b�t�@�������_�[�^�[�Q�b�g�Ƃ��Đݒ�
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

		//��ԑJ�ځF�o�b�N�o�b�t�@��\����Ԃ�
		barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			pCurrentRenderTarget,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		);
		m_pCommandList->ResourceBarrier(1, &barrier);
	}
	else {
		// ���ԃe�N�X�`�����V�F�[�_�[���\�[�X�Ƃ��Ďg�p
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_intermediateRenderTargets[m_CurrentBackBufferIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_COPY_SOURCE
		);
		m_pCommandList->ResourceBarrier(1, &barrier);
		m_beforeResourceState[m_CurrentBackBufferIndex] = D3D12_RESOURCE_STATE_COPY_SOURCE;

		//�X���b�v�`�F�[���̃o�b�N�o�b�t�@���擾
		ID3D12Resource* pCurrentRenderTarget = m_pRenderTargets[m_CurrentBackBufferIndex].Get();

		//��ԑJ�ځF�o�b�N�o�b�t�@�������_�[�^�[�Q�b�g�Ƃ��Đݒ�
		barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			pCurrentRenderTarget,
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_COPY_DEST
		);
		m_pCommandList->ResourceBarrier(1, &barrier);

		m_pCommandList->CopyResource(pCurrentRenderTarget, m_intermediateRenderTargets[m_CurrentBackBufferIndex].Get());

		//��ԑJ�ځF�o�b�N�o�b�t�@��\����Ԃ�
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

	// GPU���`����I���Ă���Α����Ɏ��̃t���[����
	/*if (m_pFence->GetCompletedValue() >= currentFanceValue) {
		m_fenceValue[m_CurrentBackBufferIndex] = currentFanceValue + 1;
		return;
	}*/

	// GPU���������̏ꍇ�͑ҋ@
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

	//�R�}���h�̋L�^���I��
	m_pCommandList->Close();

	//�R�}���h�����s
	ID3D12CommandList* ppCmdLists[] = { m_pCommandList.Get() };
	m_pQueue->ExecuteCommandLists(1, ppCmdLists);

	//�X���b�v�`�F�[����؂�ւ�
	m_pSwapChain->Present(0, 0);

	//�`�抮����҂�
	WaitRender();

	//�R�}���h�����������Ă��߂鏀��������
	m_pAllocator->Reset();
	m_pCommandList->Reset(m_pAllocator.Get(), nullptr);

	if (m_bResizeBuffer) {
		for (UINT i = 0; i < FRAME_BUFFER_COUNT; ++i)
		{
			m_pRenderTargets[i].Reset();
			m_intermediateRenderTargets[i].Reset();
		}

		//�X���b�v�`�F�C���̃o�b�t�@�T�C�Y�����T�C�Y
		HRESULT hr = m_pSwapChain->ResizeBuffers(FRAME_BUFFER_COUNT, m_windowSize.cx, m_windowSize.cy, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
		if (FAILED(hr)) {
			printf("�X���b�v�`�F�C���̃��T�C�Y�Ɏ��s���܂����B\n");
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
	//���݂̎���
	DWORD timeNow = timeGetTime();
	//�O�̃t���[�����牽�~���b�o�߂�����
	DWORD nowFrameTimeMS = timeNow - m_sceneTimeMS - m_initTime;
	//�t���[�����Ԃ�b�ɒ���
	m_frameTime = nowFrameTimeMS / 1000.0f;

	//�t���[�Y�΍�
	if (m_frameTime > 0.5f) {
		m_frameTime = 0.5f;
	}

	//�V�[�����؂�ւ���Ă���̎���(�~���b�ƕb)
	m_sceneTimeMS = timeNow - m_initTime;

	//�}�E�X�̏�Ԃ��X�V
	m_keyInput.Update();

	//�T�E���h���X�V
	m_bassSoundSystem.Update();

	//UI���X�V
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
	//�r���[�|�[�g�ƃV�U�[��`��ݒ�
	m_pCommandList->RSSetViewports(1, &m_Viewport);
	m_pCommandList->RSSetScissorRects(1, &m_Scissor);

	//���ԃe�N�X�`���p��RTV��ݒ�
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += static_cast<UINT64>((m_CurrentBackBufferIndex + FRAME_BUFFER_COUNT) * m_RtvDescriptorSize);

	//�[�x�X�e���V���r���[���擾
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_pDsvHeap->GetCPUDescriptorHandleForHeapStart();

	//RTV��DSV��ݒ�
	m_pCommandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

	//�[�x�X�e���V�����N���A
	m_pCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//���ԃe�N�X�`�����N���A
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
		printf("WindowMode��WindowNormal�̏ꍇ��windowSize���w�肷��K�v������܂��B\n");
		return;
	}

	//�{�[�_�[���X�E�B���h�E�ɐ؂�ւ�
	LONG style = 0;
	if (windowMode == WindowMode::WindowNormal || windowMode == WindowMode::WindowMaximum) {
		style |= WS_OVERLAPPEDWINDOW;
		style &= ~WS_SIZEBOX;
	}
	else {
		style = WS_OVERLAPPED;
	}

	SetWindowLong(m_hWnd, GWL_STYLE, style);

	//���j�^�[�̃T�C�Y���擾
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

		//�E�B���h�E�̈ʒu�ƃT�C�Y�𒲐�
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
