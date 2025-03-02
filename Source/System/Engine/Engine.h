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

	//�G���W��������
	bool Init(UINT windowWidth, UINT windowHeight);

	Character* AddCharacter(std::string modelFile);
	Model* AddModel(std::string modelFile);

	//�`��̊J�n����
	void BeginRender();
	void ModelRender();
	//�`��̏I������
	void EndRender();

	//�G���W���̍X�V
	void Update();
	void LateUpdate();

	void ResetViewportAndScissor();

	//�L�[������t�H�[�J�X���Ȃ���Ԃł��󂯕t���邩�ǂ�����ݒ�
	void SetKeyResponseUnFocus(bool bCanResponse);

	//�t�@�C������A�j���[�V���������[�h
	//���Ƀ��[�h�ς�
	Animation GetAnimation(std::string animFilePath);

public: //�Q�b�^�[�֐�
	//�}�E�X�̏�Ԃ��擾
	BYTE GetMouseState(BYTE keyCode);
	//�}�E�X�̃{�^����Ԃ��擾 (�������u�Ԃ̂�)
	BYTE GetMouseStateSync(BYTE keyCode);
	//�}�E�X�̈ړ��ʂ��擾
	POINT GetMouseMove();
	//�L�[�̏�Ԃ��擾
	bool GetKeyState(UINT key);
	bool GetKeyStateSync(UINT key);

	//�G���W���̃f�o�C�X
	inline ID3D12Device6* GetDevice()
	{
		return m_pDevice.Get();
	}

	//�R�}���h���X�g
	inline ID3D12GraphicsCommandList* GetCommandList()
	{
		return m_pCommandList.Get();
	}

	//�T�E���h�V�X�e��
	inline SoundSystem* GetSoundSystem()
	{
		return &m_soundSystem;
	}

	//�f�B���N�V���i�����C�g
	inline DirectionalLight* GetDirectionalLight()
	{
		return &m_directionalLight;
	}

	//�J�������擾
	inline Camera* GetCamera()
	{
		return &m_camera;
	}

	//�g���v���o�b�t�@�����O�̌��݂̃C���f�b�N�X
	inline UINT CurrentBackBufferIndex() const
	{
		return m_CurrentBackBufferIndex;
	}

	//�O��̃t���[�����牽�b�o�߂��������擾
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
	ComPtr<IDXGIFactory6> m_pDxgiFactory = nullptr;	//�t�@�N�g��(GPU�̉��)
	ComPtr<ID3D12CommandAllocator> m_pAllocator = nullptr;	//�R�}���h�A���P�[�^�[
	ComPtr<ID3D12GraphicsCommandList4> m_pCommandList = nullptr; //�R�}���h���X�g
	ComPtr<ID3D12Debug1> m_pDebugController;			//�f�o�b�O�R���g���[���[

	HANDLE m_fenceEvent = nullptr;				//�t�F���X�Ŏg���C�x���g
	ComPtr<ID3D12Fence> m_pFence = nullptr;		//�t�F���X
	UINT64 m_fenceValue[FRAME_BUFFER_COUNT];	//�t�F���X�̒l�i�g���v���o�b�t�@�����O�p��3�j
	D3D12_VIEWPORT m_Viewport;					//�r���[�|�[�g
	D3D12_RECT m_Scissor;						//�V�U�[��`
	Input m_keyInput;							//�L�[���

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
	ComPtr<ID3D12Resource> m_pDepthStencilBuffer = nullptr;

	ModelManager m_modelManager;
	MaterialManager m_materialManager;
	ZShadow m_zShadow;

private: //�`�惋�[�v�Ŏg�p�������
	//�`�抮����҂���
	void WaitRender();

private: //�v���C�x�[�g�ϐ�
	unsigned long m_initTime;
	unsigned long m_sceneTimeMS;
	float m_frameTime;

	AnimationManager animManager;

	SoundSystem m_soundSystem;

	DirectionalLight m_directionalLight;

	Camera m_camera;
};

//�ǂ�����ł��Q�Ƃ��邽�߃O���[�o���ϐ�
extern Engine* g_Engine;
