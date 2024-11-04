#include "Engine.h"
#include <d3d12.h>
#include <stdio.h>
#include <Windows.h>

Engine* g_Engine;

Engine::Engine()
	: m_modelManager(ModelManager())
	, m_camera(Camera())
	, m_Scissor(D3D12_RECT())
	, m_Viewport(D3D12_VIEWPORT())
	, m_fenceValue{0, 0, 0}
	, m_frameTime(0.0f)
	, m_hWnd(nullptr)
	, m_initTime(0)
	, m_pDirectionalLight(nullptr)
	, m_pKeyInput(nullptr)
	, m_pSoundSystem(nullptr)
	, m_sceneTimeMS(0)
{
}

Engine::~Engine()
{
	delete m_pKeyInput;
	delete m_pSoundSystem;
}

bool Engine::Init(HWND hwnd, UINT windowWidth, UINT windowHeight)
{
	m_FrameBufferWidth = windowWidth;
	m_FrameBufferHeight = windowHeight;
	m_hWnd = hwnd;

	if (!CreateDevice())
	{
		printf("�f�o�C�X�̐����Ɏ��s");
		return false;
	}
	if (!CreateCommandQueue())
	{
		printf("�R�}���h�L���[�̐����Ɏ��s");
		return false;
	}

	if (!CreateSwapChain())
	{
		printf("�X���b�v�`�F�C���̐����Ɏ��s");
		return false;
	}

	if (!CreateCommandList())
	{
		printf("�R�}���h���X�g�̐����Ɏ��s");
		return false;
	}

	if (!CreateFence())
	{
		printf("�t�F���X�̐����Ɏ��s");
		return false;
	}

	if (!CreateRenderTarget())
	{
		printf("�����_�[�^�[�Q�b�g�̐����Ɏ��s");
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

	m_pKeyInput = new Input(m_hWnd);

	//FrameTime�v���p
	m_initTime = timeGetTime();
	m_sceneTimeMS = 0;
	m_frameTime = 0.0f;

	//�T�E���h�V�X�e����������
	m_pSoundSystem = new SoundSystem(m_hWnd);

	//�f�B���N�V���i�����C�g�̏�����
	m_pDirectionalLight = new DirectionalLight();

	//�e�p�̃��[�g�V�O�l�`���A�p�C�v���C���X�e�[�g�̍쐬
	m_pShadowRootSignature = new RootSignature(m_pDevice.Get(), ShaderKinds::ShadowShader);
	m_pShadowPipelineState = new PipelineState(m_pDevice.Get(), m_pShadowRootSignature);

	m_pShadowDescriptorHeap = new DescriptorHeap(m_pDevice.Get(), 1, ShadowSizeHigh);

	printf("�`��G���W���̏������ɐ���\n");
	return true;
}

Character* Engine::AddCharacter(std::string modelFile)
{
	Character* pCharacter = new Character(modelFile, m_pDevice.Get(), m_pCommandList.Get(), &m_camera, m_pDirectionalLight, m_pDepthStencilBuffer.Get());
	//pCharacter->SetShadowMap(m_pShadowDescriptorHeap->GetShadowMap());
	m_modelManager.AddModel(pCharacter);
	return pCharacter;
}

Model* Engine::AddModel(std::string modelFile)
{
	Model* pModel = new Model(m_pDevice.Get(), m_pCommandList.Get(), &m_camera, m_pDirectionalLight, m_pDepthStencilBuffer.Get());
	//pModel->SetShadowMap(m_pShadowDescriptorHeap->GetShadowMap());
	pModel->LoadModel(modelFile);
	m_modelManager.AddModel(pModel);
	return pModel;
}

bool Engine::CreateDevice()
{
	auto hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_pDevice.ReleaseAndGetAddressOf()));
	return SUCCEEDED(hr);
}

bool Engine::CreateCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

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
	HRESULT hr;
	for (size_t i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		hr = m_pDevice->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(m_pAllocator[i].ReleaseAndGetAddressOf()));
	}

	if (FAILED(hr))
	{
		return false;
	}

	//�R�}���h���X�g�̐���
	hr = m_pDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_pAllocator[m_CurrentBackBufferIndex].Get(),
		nullptr,
		IID_PPV_ARGS(&m_pCommandList)
	);

	if (FAILED(hr))
	{
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
	m_currentRenderTarget = m_pRenderTargets[m_CurrentBackBufferIndex].Get();

	//�R�}���h�����������Ă��߂鏀��������
	m_pAllocator[m_CurrentBackBufferIndex]->Reset();
	m_pCommandList->Reset(m_pAllocator[m_CurrentBackBufferIndex].Get(), nullptr);

	//�r���[�|�[�g�ƃV�U�[��`��ݒ�
	m_pCommandList->RSSetViewports(1, &m_Viewport);
	m_pCommandList->RSSetScissorRects(1, &m_Scissor);

	//���݂̃t���[���̃����_�[�^�[�Q�b�g�r���[�̃f�B�X�N���v�^�q�[�v�̊J�n�A�h���X���擾
	auto currentRtvHandle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();
	currentRtvHandle.ptr += static_cast<SIZE_T>(m_CurrentBackBufferIndex * m_RtvDescriptorSize);

	//�[�x�X�e���V���̃f�B�X�N���v�^�q�[�v�̊J�n�A�h���X�擾
	auto currentDsvHandle = m_pDsvHeap->GetCPUDescriptorHandleForHeapStart();

	//�����_�[�^�[�Q�b�g���g�p�\�ɂȂ�܂ő҂�
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_currentRenderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_pCommandList->ResourceBarrier(1, &barrier);

	//�[�x�X�e���V���r���[���N���A
	m_pCommandList->ClearDepthStencilView(currentDsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//�����_�[�^�[�Q�b�g��ݒ�
	m_pCommandList->OMSetRenderTargets(1, &currentRtvHandle, FALSE, &currentDsvHandle);

	//�����_�[�^�[�Q�b�g���N���A
	const float clearColor[] = { 0.25f, 0.25f, 0.25f, 1.0f };
	m_pCommandList->ClearRenderTargetView(currentRtvHandle, clearColor, 0, nullptr);
}

void Engine::ModelRender()
{
	ID3D12GraphicsCommandList* pCommandList = m_pCommandList.Get();

	//�V���h�E�}�b�v�������_�[�^�[�Q�b�g�Ƃ��Ďg�p���鏀��
	/*CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pShadowDescriptorHeap->GetShadowMap(),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_STATE_DEPTH_WRITE);
	pCommandList->ResourceBarrier(1, &barrier);

	pCommandList->ClearDepthStencilView(
		*m_pShadowDescriptorHeap->GetShadowMapDSV(),  //�V���h�E�}�b�v��DSV
		D3D12_CLEAR_FLAG_DEPTH,
		1.0f,  //�[�x�N���A�l (�ő�l�ɐݒ�)
		0,     //�X�e���V���N���A�l
		0, nullptr);*/

	//�V���h�E�}�b�v�p�̃p�C�v���C���X�e�[�g�ƃ��[�g�V�O�l�`����ݒ�

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	m_pCommandList->ResourceBarrier(1, &barrier);

	pCommandList->SetPipelineState(m_pShadowPipelineState->GetPipelineState());
	pCommandList->SetGraphicsRootSignature(m_pShadowRootSignature->GetRootSignature());

	//�r���[�|�[�g�ƃV�U�[��`�̐ݒ�
	//pCommandList->RSSetViewports(1, m_pShadowDescriptorHeap->GetShadowViewPort());
	//pCommandList->RSSetScissorRects(1, m_pShadowDescriptorHeap->GetShadowScissor());

	//�[�x�o�b�t�@�̐ݒ�
	//pCommandList->OMSetRenderTargets(0, nullptr, false, m_pShadowDescriptorHeap->GetShadowMapDSV());

	//���f���̉e�̕`��
	m_modelManager.RenderShadowMap(m_CurrentBackBufferIndex);

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	pCommandList->ResourceBarrier(1, &barrier);

	//ResetViewportAndScissor();

	//���f����`��
	m_modelManager.RenderModel(m_CurrentBackBufferIndex);
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
		//�������ɃC�x���g��ݒ�.
		auto hr = m_pFence->SetEventOnCompletion(currentFanceValue, m_fenceEvent);
		if (FAILED(hr))
		{
			return;
		}

		//�ҋ@����.
		if (WAIT_OBJECT_0 != WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE))
		{
			return;
		}

		m_fenceValue[m_CurrentBackBufferIndex] = currentFanceValue + 1;
	}
}

void Engine::EndRender()
{
	//�����_�[�^�[�Q�b�g�ɏ������ݏI���܂ő҂�
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_currentRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
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
}

BYTE Engine::GetMouseState(BYTE keyCode)
{
	return m_pKeyInput->GetMouseState(keyCode);
}

BYTE Engine::GetMouseStateSync(BYTE keyCode)
{
	return m_pKeyInput->GetMouseStateSync(keyCode);
}

bool Engine::GetKeyState(UINT key)
{
	return m_pKeyInput->CheckKey(key);
}

bool Engine::GetKeyStateSync(UINT key)
{
	return m_pKeyInput->TriggerKey(key);
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
	m_pKeyInput->UpdateMouseState();

	//�T�E���h���X�V
	m_pSoundSystem->Update();
}

void Engine::LateUpdate()
{
	m_camera.Update(m_pDirectionalLight);

	m_modelManager.Update(m_CurrentBackBufferIndex);
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
	m_pCommandList->OMSetRenderTargets(1, &currentRtvHandle, false, &currentDsvHandle);
}

Animation Engine::GetAnimation(std::string animFilePath)
{
	if (!std::filesystem::exists(animFilePath))
		return Animation();

	return animManager.LoadAnimation(animFilePath);
}
