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

	//�G���W��������
	bool Init(int windowWidth, int windowHeight);

	//�t�@�C������3D���f��(�{�[���t��)��ǂݍ���
	Character* AddCharacter(std::string modelFile);
	//�t�@�C������3D���f����ǂݍ���
	Model* AddModel(std::string modelFile);
	//�N�A�b�h���f����ǂݍ���
	Model* AddPrimitiveQuad();

	//���f�������
	void ReleaseModel(Model* pModel);

	//�`��̊J�n����
	void Begin3DRender();
	//3D��`��
	void ModelRender();
	//2D��`��
	void Begin2DRender();
	void ApplyPostProcess();
	//�`��̏I������
	void EndRender();

	//�G���W���̍X�V
	void Update();
	void LateUpdate();

	void SetEnablePostProcess(bool value);

	void ResetViewportAndScissor();

	//�L�[������t�H�[�J�X���Ȃ���Ԃł��󂯕t���邩�ǂ�����ݒ�
	void SetKeyResponseUnFocus(bool bCanResponse);

	//�o�b�N�J���[��ݒ�
	void SetClearColor(DXGI_RGBA clearColor);

	//�E�B���h�E�̃X�^�C����ύX
	//���� : WindowMode ���[�h, SIZE �E�B���h�E�T�C�Y
	//��windowSize��WindowMode = WindowNormal�̂ݎw��
	void SetWindowMode(WindowMode windowMode, _In_opt_ SIZE* windowSize);

	//�t�@�C������A�j���[�V���������[�h
	//���Ƀ��[�h�ς�
	Animation* GetAnimation(std::string animFilePath);

public: //�Q�b�^�[�֐�
	//�}�E�X�̏�Ԃ��擾
	BYTE GetMouseState(BYTE keyCode) const;
	//�}�E�X�̃{�^����Ԃ��擾 (�������u�Ԃ̂�)
	BYTE GetMouseStateSync(BYTE keyCode);
	//�}�E�X�̃{�^����Ԃ��擾 (�������u�Ԃ̂�)
	BYTE GetMouseStateRelease(BYTE keyCode);

	//�}�E�X�̈ړ��ʂ��擾
	POINT GetMouseMove() const;
	//�}�E�X���W���擾
	POINT GetMousePosition() const;

	//�L�[�̏�Ԃ��擾 (�������ςȂ�)
	bool GetKeyState(UINT key);
	//�L�[�̏�Ԃ��擾 (�������u�Ԃ̂�)
	bool GetKeyStateSync(UINT key);

	inline HWND GetHWND() const { return m_hWnd; }

	//�G���W���̃f�o�C�X
	inline ID3D12Device6* GetDevice() const { return m_pDevice.Get(); }

	//�R�}���h���X�g
	inline ID3D12GraphicsCommandList4* GetCommandList() const { return m_pCommandList.Get(); }

	//Bass Audio Library - �T�E���h�V�X�e��
	inline BassSoundSystem* GetBassSoundSystem() { return &m_bassSoundSystem; }

	//Wwise - �T�E���h�V�X�e��
	inline WwiseSoundSystem* GetWwiseSoundSystem() { return &m_wwiseSoundSystem; }

	//�f�B���N�V���i�����C�g
	inline DirectionalLight* GetDirectionalLight() { return &m_directionalLight; }

	//�J�������擾
	inline Camera* GetCamera() { return &m_camera; }

	//�}�e���A���V�X�e��
	inline MaterialManager* GetMaterialManager() { return &m_materialManager; }

	//�G�t�F�N�g�V�X�e��
	inline EffectManager* GetEffectManager() { return &m_effectManager; }

	//�X�J�C�{�b�N�X
	inline SkyBox* GetSkyBox() { return &m_skyBox; }

	//�e
	inline ZShadow* GetZShadow() { return &m_zShadow; }

	//UI�V�X�e��
	inline UIManager* GetUIManager() { return &m_uiManager; }

	//�g���v���o�b�t�@�����O�̌��݂̃C���f�b�N�X
	inline UINT CurrentBackBufferIndex() const { return m_CurrentBackBufferIndex; }

	//�|�X�g�v���Z�X���L�����ǂ���
	inline bool GetIsPostProcessEnabled() const { return m_bEnablePostProcess; }

	//�O��̃t���[�����牽�b�o�߂��������擾
	inline float GetFrameTime() const { return m_frameTime; }

	//�w�i�F���擾
	inline DXGI_RGBA GetClearColor() const { return m_clearColor; }

	//���݂̃E�B���h�E���[�h���擾
	inline WindowMode GetWindowMode() const { return m_windowMode; }

	//���݂̃E�B���h�E�T�C�Y���擾
	inline SIZE GetWindowSize() const { return m_windowSize; }

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

	//�`�抮����҂���
	void WaitRender();

private: //�`��Ɏg��DirectX12�̃I�u�W�F�N�g
	HWND m_hWnd;
	UINT m_CurrentBackBufferIndex = 0;

	ComPtr<ID3D12Device6> m_pDevice = nullptr;		//�f�o�C�X
	ComPtr<ID3D12CommandQueue> m_pQueue = nullptr;	//�R�}���h�L���[
	ComPtr<IDXGISwapChain3> m_pSwapChain = nullptr; //�X���b�v�`�F�C��
	ComPtr<IDXGIFactory6> m_pDxgiFactory = nullptr;	//�t�@�N�g��(GPU�̉��)
	ComPtr<ID3D12CommandAllocator> m_pAllocator = nullptr;	//�R�}���h�A���P�[�^�[
	ComPtr<ID3D12GraphicsCommandList4> m_pCommandList = nullptr; //�R�}���h���X�g
	ComPtr<ID3D12Debug1> m_pDebugController;			//�f�o�b�O�R���g���[���[
	ComPtr<ID3D12Fence> m_pFence = nullptr;		//�t�F���X

	HANDLE m_fenceEvent = nullptr;				//�t�F���X�Ŏg���C�x���g
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
	ComPtr<ID3D12Resource> m_intermediateRenderTargets[FRAME_BUFFER_COUNT] = { nullptr };

	//�[�x�X�e���V���̃f�B�X�N���v�^�[�T�C�Y
	UINT m_DsvDescriptorSize = 0;
	//�[�x�X�e���V���̃f�B�X�N���v�^�q�[�v
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

private: //�v���C�x�[�g�ϐ�
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

//�ǂ�����ł��Q�Ƃ��邽�߃O���[�o���ϐ�
extern Engine* g_Engine;
