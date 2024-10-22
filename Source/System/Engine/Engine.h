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

#pragma comment(lib, "d3d12.lib") // d3d12���C�u�����������N����
#pragma comment(lib, "dxgi.lib") // dxgi���C�u�����������N����

constexpr int FRAME_BUFFER_COUNT = 3;

class Engine
{
public:
	~Engine();

	//�G���W��������
	bool Init(HWND hwnd, UINT windowWidth, UINT windowHeight);

	//�`��̊J�n����
	void BeginRender();
	//�`��̏I������
	void EndRender();

	//�G���W���̍X�V
	void Update();

	//�t�@�C������A�j���[�V���������[�h
	//���Ƀ��[�h�ς�
	Animation GetAnimation(std::string animFilePath);

public: //�Q�b�^�[�֐�
	//�}�E�X�̏�Ԃ��擾
	BYTE GetMouseState(BYTE keyCode);
	//�}�E�X�̃{�^����Ԃ��擾 (�������u�Ԃ̂�)
	BYTE GetMouseStateSync(BYTE keyCode);
	//�L�[�̏�Ԃ��擾
	bool GetKeyState(UINT key);
	bool GetKeyStateSync(UINT key);

	//�G���W���̃f�o�C�X
	inline ID3D12Device6* Device()
	{
		return m_pDevice.Get();
	}

	//�R�}���h���X�g
	inline ID3D12GraphicsCommandList* CommandList()
	{
		return m_pCommandList.Get();
	}

	//�T�E���h�V�X�e��
	inline SoundSystem* GetSoundSystem()
	{
		return m_pSoundSystem;
	}

	//�f�B���N�V���i�����C�g
	inline DirectionalLight* GetDirectionalLight()
	{
		return m_pDirectionalLight;
	}

	//�g���v���o�b�t�@�����O�̌��݂̃C���f�b�N�X
	inline UINT CurrentBackBufferIndex() const
	{
		return m_CurrentBackBufferIndex;
	}

	//�O��̃t���[�����牽�~���b�o�߂��������擾
	inline float GetFrameTime() const
	{
		return m_frameTime;
	}

private: //DirectX12�̏�����
	//�f�o�C�X���쐬
	bool CreateDevice();
	//�R�}���h�L���[�𐶐�
	bool CreateCommandQueue();
	//�X���b�v�`�F�C���𐶐� (�g���v���o�b�t�@�����O : ��ʂ̂������}����)
	bool CreateSwapChain();
	//�R�}���h���X�g�ƃR�}���h�A���P�[�^�[�𐶐�
	bool CreateCommandList();
	//�t�F���X�𐶐�
	bool CreateFence();
	//�r���[�|�[�g�𐶐�
	void CreateViewPort();
	//�V�U�[��`�𐶐�
	void CreateScissorRect();

private: //�`��Ɏg��DirectX12�̃I�u�W�F�N�g
	HWND m_hWnd;
	UINT m_FrameBufferWidth = 0;
	UINT m_FrameBufferHeight = 0;
	UINT m_CurrentBackBufferIndex = 0;

	ComPtr<ID3D12Device6> m_pDevice = nullptr;		//�f�o�C�X
	ComPtr<ID3D12CommandQueue> m_pQueue = nullptr;	//�R�}���h�L���[
	ComPtr<IDXGISwapChain3> m_pSwapChain = nullptr; //�X���b�v�`�F�C��
	ComPtr<ID3D12CommandAllocator> m_pAllocator[FRAME_BUFFER_COUNT] = { nullptr };	//�R�}���h�A���P�[�^�[
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList = nullptr;						//�R�}���h���X�g
	HANDLE m_fenceEvent = nullptr;				//�t�F���X�Ŏg���C�x���g
	ComPtr<ID3D12Fence> m_pFence = nullptr;		//�t�F���X
	UINT64 m_fenceValue[FRAME_BUFFER_COUNT];	//�t�F���X�̒l�i�g���v���o�b�t�@�����O�p��3�j
	D3D12_VIEWPORT m_Viewport;					//�r���[�|�[�g
	D3D12_RECT m_Scissor;						//�V�U�[��`
	Input* m_pKeyInput;							//�L�[���

private: //�`��Ɏg���I�u�W�F�N�g
	//�����_�[�^�[�Q�b�g�𐶐�
	bool CreateRenderTarget();
	//�[�x�X�e���V���o�b�t�@�𐶐�
	bool CreateDepthStencil();

	//�����_�[�^�[�Q�b�g�r���[�̃f�B�X�N���v�^�T�C�Y
	UINT m_RtvDescriptorSize = 0;
	//�����_�[�^�[�Q�b�g�̃f�B�X�N���v�^�q�[�v
	ComPtr<ID3D12DescriptorHeap> m_pRtvHeap = nullptr;
	//�����_�[�^�[�Q�b�g�i�g���v���o�b�t�@�����O�p��3�j
	ComPtr<ID3D12Resource> m_pRenderTargets[FRAME_BUFFER_COUNT] = { nullptr };

	//�[�x�X�e���V���̃f�B�X�N���v�^�[�T�C�Y
	UINT m_DsvDescriptorSize = 0;
	//�[�x�X�e���V���̃f�B�X�N���v�^�q�[�v
	ComPtr<ID3D12DescriptorHeap> m_pDsvHeap = nullptr;
	//�[�x�X�e���V���o�b�t�@
	ComPtr<ID3D12Resource> m_pDepthStencilBuffer = nullptr;

private: //�`�惋�[�v�Ŏg�p�������
	//���݂̃t���[���̃����_�[�^�[�Q�b�g���ꎞ�I�ɕۑ�
	ID3D12Resource* m_currentRenderTarget = nullptr;

	//�`�抮����҂���
	void WaitRender();

private: //�v���C�x�[�g�ϐ�
	unsigned long m_initTime;
	unsigned long m_sceneTimeMS;
	float m_frameTime;

	AnimationManager animManager;

	SoundSystem* m_pSoundSystem;

	DirectionalLight* m_pDirectionalLight;
};

//�ǂ�����ł��Q�Ƃ��邽�߃O���[�o���ϐ�
extern Engine* g_Engine;
