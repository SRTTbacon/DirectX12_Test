#include "Engine.h"
#include <d3d12.h>
#include <stdio.h>
#include <Windows.h>

Engine* g_Engine = nullptr;

Engine::Engine(HWND hwnd)
	: m_modelManager(ModelManager())
	, m_camera(Camera())
	, m_Scissor(D3D12_RECT())
	, m_Viewport(D3D12_VIEWPORT())
	, m_fenceValue{0}
	, m_frameTime(0.0f)
	, m_hWnd(hwnd)
	, m_initTime(0)
	, m_directionalLight(DirectionalLight())
	, m_keyInput(hwnd)
	, m_soundSystem(hwnd)
	, m_sceneTimeMS(0)
	, m_zShadow(ZShadow())
{
}

Engine::~Engine()
{
}

void Engine::Release()
{
	m_pCommandList->Close();
}

bool Engine::Init(UINT windowWidth, UINT windowHeight)
{
	m_FrameBufferWidth = windowWidth;
	m_FrameBufferHeight = windowHeight;

#ifdef _DEBUG
	//�f�o�b�O���C���[�̐ݒ�
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_pDebugController)))) {
		m_pDebugController->SetEnableGPUBasedValidation(TRUE);
		m_pDebugController->SetEnableSynchronizedCommandQueueValidation(TRUE);
		m_pDebugController->EnableDebugLayer();
		printf("�f�o�b�O���C���[��L�������܂����B\n");
	}
#endif

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

	//�r���[�|�[�g�ƃV�U�[��`�𐶐�
	CreateViewPort();
	CreateScissorRect();

	//FrameTime�v���p
	m_initTime = timeGetTime();
	m_sceneTimeMS = 0;
	m_frameTime = 0.0f;

	//�f�B���N�V���i�����C�g�̏�����
	m_directionalLight.Init(m_pDevice.Get());

	m_zShadow.Init(m_pDevice.Get(), m_pCommandList.Get());

	//�R�}���h�����������Ă��߂鏀��������
	m_pAllocator->Reset();
	m_pCommandList->Reset(m_pAllocator.Get(), nullptr);

	srand(static_cast<UINT>(timeGetTime()));

	printf("�`��G���W���̏������ɐ���\n");
	return true;
}

Character* Engine::AddCharacter(std::string modelFile)
{
	Character* pCharacter = new Character(modelFile, m_pDevice.Get(), m_pCommandList.Get(), &m_camera, &m_directionalLight, m_zShadow.GetZBuffer());
	m_modelManager.AddModel(pCharacter);
	return pCharacter;
}

Model* Engine::AddModel(std::string modelFile)
{
	Model* pModel = new Model(m_pDevice.Get(), m_pCommandList.Get(), &m_camera, &m_directionalLight, m_zShadow.GetZBuffer());
	pModel->LoadModel(modelFile);
	m_modelManager.AddModel(pModel);
	return pModel;
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
	desc.BufferDesc.Width = m_FrameBufferWidth;
	desc.BufferDesc.Height = m_FrameBufferHeight;
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
	m_Viewport.Width = static_cast<float>(m_FrameBufferWidth);
	m_Viewport.Height = static_cast<float>(m_FrameBufferHeight);
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;
}

void Engine::CreateScissorRect()
{
	m_Scissor.left = 0;
	m_Scissor.right = m_FrameBufferWidth;
	m_Scissor.top = 0;
	m_Scissor.bottom = m_FrameBufferHeight;
}

bool Engine::CreateRenderTarget()
{
	//RTV�p�̃f�B�X�N���v�^�q�[�v���쐬����
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = FRAME_BUFFER_COUNT;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	auto hr = m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_pRtvHeap.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		return false;
	}

	//�f�B�X�N���v�^�̃T�C�Y���擾�B
	m_RtvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_pRenderTargets[i].ReleaseAndGetAddressOf()));
		m_pDevice->CreateRenderTargetView(m_pRenderTargets[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += m_RtvDescriptorSize;
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
		m_FrameBufferWidth,
		m_FrameBufferHeight,
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

void Engine::BeginRender()
{
	//���݂̃����_�[�^�[�Q�b�g���X�V
	ID3D12Resource* pCurrentRenderTarget = m_pRenderTargets[m_CurrentBackBufferIndex].Get();

	//�r���[�|�[�g�ƃV�U�[��`��ݒ�
	m_pCommandList->RSSetViewports(1, &m_Viewport);
	m_pCommandList->RSSetScissorRects(1, &m_Scissor);

	//���݂̃t���[���̃����_�[�^�[�Q�b�g�r���[�̃f�B�X�N���v�^�q�[�v�̊J�n�A�h���X���擾
	auto currentRtvHandle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();
	currentRtvHandle.ptr += static_cast<SIZE_T>(m_CurrentBackBufferIndex * m_RtvDescriptorSize);

	//�[�x�X�e���V���̃f�B�X�N���v�^�q�[�v�̊J�n�A�h���X�擾
	auto currentDsvHandle = m_pDsvHeap->GetCPUDescriptorHandleForHeapStart();

	//�����_�[�^�[�Q�b�g���g�p�\�ɂȂ�܂ő҂�
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(pCurrentRenderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_pCommandList->ResourceBarrier(1, &barrier);
}

void Engine::ModelRender()
{
	//���f���̉e�̕`��
	m_zShadow.BeginMapping();
	m_modelManager.RenderShadowMap(m_CurrentBackBufferIndex);
	
	ResetViewportAndScissor();

	//�[�x�}�b�v��ύX
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = m_zShadow.GetZBuffer();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_pCommandList->ResourceBarrier(1, &barrier);
	
	//���f����`��
	m_modelManager.RenderModel(m_CurrentBackBufferIndex);

	//�[�x�}�b�v��ύX
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	m_pCommandList->ResourceBarrier(1, &barrier);
}

void Engine::WaitRender()
{
	//�`��I���҂�
	const UINT64 currentFanceValue = m_fenceValue[m_CurrentBackBufferIndex];
	m_pQueue->Signal(m_pFence.Get(), currentFanceValue);

	//�o�b�N�o�b�t�@�ԍ��X�V
	m_CurrentBackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();


	//���̃t���[���̕`�揀�����܂��ł���Αҋ@����.
	if (m_pFence->GetCompletedValue() < currentFanceValue)
	{
		//�������ɃC�x���g��ݒ�
		HRESULT hr = m_pFence->SetEventOnCompletion(currentFanceValue, m_fenceEvent);
		if (FAILED(hr))
		{
			return;
		}

		//�ҋ@����
		WaitForSingleObject(m_fenceEvent, INFINITE);

		m_fenceValue[m_CurrentBackBufferIndex] = currentFanceValue + 1;
	}
}

void Engine::EndRender()
{
	//�����_�[�^�[�Q�b�g�ɏ������ݏI���܂ő҂�
	ID3D12Resource* pCurrentRenderTarget = m_pRenderTargets[m_CurrentBackBufferIndex].Get();
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(pCurrentRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_pCommandList->ResourceBarrier(1, &barrier);

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
}

BYTE Engine::GetMouseState(BYTE keyCode)
{
	return m_keyInput.GetMouseState(keyCode);
}

BYTE Engine::GetMouseStateSync(BYTE keyCode)
{
	return m_keyInput.GetMouseStateSync(keyCode);
}

POINT Engine::GetMouseMove()
{
	return m_keyInput.GetMouseMove();
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
	//�V�[�����؂�ւ���Ă���̎���(�~���b�ƕb)
	m_sceneTimeMS = timeNow - m_initTime;

	//�}�E�X�̏�Ԃ��X�V
	m_keyInput.UpdateMouseState();

	//�T�E���h���X�V
	m_soundSystem.Update();
}

void Engine::LateUpdate()
{
	m_camera.Update(&m_directionalLight);

	m_directionalLight.Update();

	m_modelManager.LateUpdate(m_CurrentBackBufferIndex);
}

void Engine::ResetViewportAndScissor()
{
	//�r���[�|�[�g�ƃV�U�[��`��ݒ�
	m_pCommandList->RSSetViewports(1, &m_Viewport);
	m_pCommandList->RSSetScissorRects(1, &m_Scissor);

	//���݂̃t���[���̃����_�[�^�[�Q�b�g�r���[�̃f�B�X�N���v�^�q�[�v�̊J�n�A�h���X���擾
	auto currentRtvHandle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();
	currentRtvHandle.ptr += static_cast<SIZE_T>(m_CurrentBackBufferIndex * m_RtvDescriptorSize);

	//�[�x�X�e���V���̃f�B�X�N���v�^�q�[�v�̊J�n�A�h���X�擾
	auto currentDsvHandle = m_pDsvHeap->GetCPUDescriptorHandleForHeapStart();

	//�����_�[�^�[�Q�b�g��ݒ�
	m_pCommandList->OMSetRenderTargets(1, &currentRtvHandle, true, &currentDsvHandle);

	//�[�x�X�e���V���r���[���N���A
	m_pCommandList->ClearDepthStencilView(currentDsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//�����_�[�^�[�Q�b�g���N���A
	const float clearColor[] = { 0.25f, 0.25f, 0.25f, 1.0f };
	m_pCommandList->ClearRenderTargetView(currentRtvHandle, clearColor, 0, nullptr);
}

Animation Engine::GetAnimation(std::string animFilePath)
{
	if (!std::filesystem::exists(animFilePath))
		return Animation();

	return animManager.LoadAnimation(animFilePath);
}
