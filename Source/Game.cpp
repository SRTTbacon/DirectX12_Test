#include "Game.h"

using namespace Common;

//=============================================================================
// �R���X�g���N�^
//=============================================================================
Game::Game()
	: m_pNowScene(nullptr)
{
}

//=============================================================================
// �f�X�g���N�^
//=============================================================================
Game::~Game()
{
	//���݂̃V�[���N���X�̉��
	if (m_pNowScene) {
		delete m_pNowScene;
		m_pNowScene = nullptr;
	}

	//�V�[���̃X�^�b�N���
	if (!Scene::m_stkScene.empty()) {

		while (!Scene::m_stkScene.empty()) {
			delete Scene::m_stkScene.top().m_pPrevScene;
			Scene::m_stkScene.top().m_pPrevScene = nullptr;
			Scene::m_stkScene.pop();
		}
	}

	//�G���W���N���X�̉��
	if (g_Engine) {
		delete g_Engine;
		g_Engine = nullptr;
	}
	if (g_resourceCopy) {
		delete g_resourceCopy;
		g_resourceCopy = nullptr;
	}
}

//=============================================================================
// �Q�[���̏�����
// ���@���Fconst HWND & �E�B���h�E�n���h��
// �@�@�@�@const HINSTANCE & �C���X�^���X�n���h��
//=============================================================================
void Game::Initialize(const HWND& hAppWnd)
{
	srand((unsigned int)time(nullptr));

	try {

		//�G���W���N���X�̏�����
		g_resourceCopy = new ResourceCopy();
		g_Engine = new Engine(hAppWnd);

		if (!g_Engine->Init(static_cast<int>(WINDOW_WIDTH), static_cast<int>(WINDOW_HEIGHT))) {
			throw DxSystemException(DxSystemException::OM_ENGINE_INITIALIZE_ERROR);
		}
	}
	catch (DxSystemException dxSystemException) {

		dxSystemException.ShowOriginalMessage();

		throw DxSystemException(DxSystemException::OM_ENGINE_INITIALIZE_ERROR);
	}
	catch (std::bad_alloc) {

		throw DxSystemException(DxSystemException::OM_ENGINE_ALLOCATE_ERROR);
	}

	//�ŏ��̃V�[���ݒ�

	Scene::m_prevSceneData.Set(SCENE_INIT, false, nullptr);

	Scene::m_nowSceneData = Scene::m_prevSceneData;

	try {
		m_pNowScene = new SceneInit();
	}
	catch (std::bad_alloc) {
		throw DxSystemException(DxSystemException::OM_NEW_ERROR);
	}
}

//-----------------------------------------------------------------------------
// �V�[���؂�ւ�
//-----------------------------------------------------------------------------
bool Game::ChangeScene()
{
	switch (Scene::m_nowSceneData.m_scene) {

	case SCENE_INIT:
		m_pNowScene = new SceneInit();
		break;

	case SCENE_TITLE:
		m_pNowScene = new SceneTitle();
		break;

	case SCENE_GAME:
		m_pNowScene = new SceneGame();
		break;

	case SCENE_CLEAR:
		m_pNowScene = new SceneClear();
		break;

	case SCENE_GAMEOVER:
		m_pNowScene = new SceneGameover();
		break;

	case SCENE_EXIT:
		return false;
	}

	return true;
}

//=============================================================================
// ���s
//=============================================================================
bool Game::Run()
{
	try {

		//�V�[�����ς������
		if (Scene::m_nowSceneData.m_scene != Scene::m_prevSceneData.m_scene) {

			//SCENE_EXIT�ŏI��
			if (!ChangeScene()) {
				return false;
			}

			Scene::m_prevSceneData = Scene::m_nowSceneData;

			SetCursor(LoadCursor(NULL, IDC_ARROW));
		}

		//Run�̖߂�l��true�i�I���j�@���@�V�[�����ς����
		if (m_pNowScene->Run()) {

			//�X�^�b�N�𒲂ׂ�
			if (Scene::m_stkScene.empty()) {

				//�X�^�b�N����i���݁A�T�u�V�[���ł͂Ȃ��j

				//�V�����V�[�����T�u�V�[���Ŗ������
				if (!Scene::m_nowSceneData.m_bSubScene) {

					//�V�[���̉��
					delete m_pNowScene;
					m_pNowScene = nullptr;

				}
				else {

					//�V�����V�[�����T�u�V�[���̏ꍇ

					//���݂̃V�[����ۑ�
					Scene::m_prevSceneData.m_pPrevScene = m_pNowScene;

					//�X�^�b�N�Ƀv�b�V��
					Scene::m_stkScene.push(Scene::m_prevSceneData);
				}
			}
			else {

				//�X�^�b�N�ɃV�[����񂪂���i���݃T�u�V�[���ɓ����Ă���j

				//�X�^�b�N�̃g�b�v�����V�[���@���@�V�[����߂�
				if (Scene::m_nowSceneData.m_scene == Scene::m_stkScene.top().m_scene) {

					//�V�[���̉��
					delete m_pNowScene;
					m_pNowScene = nullptr;

					//�ۑ����Ă������O�̃V�[����߂�
					m_pNowScene = Scene::m_stkScene.top().m_pPrevScene;

					//�X�^�b�N����O�̃V�[��������
					Scene::m_stkScene.pop();

					//�T�u�V�[������߂��������N���X�ɓ`����
					m_pNowScene->ReturnSubScene();

					Scene::m_prevSceneData = Scene::m_nowSceneData;
				}
				else {

					//�X�^�b�N�̃g�b�v�ƈႤ�V�[��

					//SCENE_EXIT�ł���ΏI��
					if (Scene::m_nowSceneData.m_scene == SCENE_EXIT) {
						return false;
					}

					//�T�u�V�[���ȊO�ւ̑J��
					if (!Scene::m_nowSceneData.m_bSubScene) {

						//���݂̃V�[�������
						delete m_pNowScene;
						m_pNowScene = nullptr;

						//�X�^�b�N�ɂ���V�[����S�č폜����
						while (true) {

							delete Scene::m_stkScene.top().m_pPrevScene;
							Scene::m_stkScene.top().m_pPrevScene = nullptr;

							Scene::m_stkScene.pop();

							if (Scene::m_stkScene.empty()) {
								break;
							}
						}
					}
					else {

						//�T�u�V�[����

						//���݂̃V�[����ۑ�
						Scene::m_prevSceneData.m_pPrevScene = m_pNowScene;

						//�X�^�b�N�Ƀv�b�V��
						Scene::m_stkScene.push(Scene::m_prevSceneData);
					}
				}
			}
		}
	}
	catch (DxSystemException dxSystemExeption)
	{
		dxSystemExeption.ShowOriginalMessage();

		return false;
	}
	catch (std::bad_alloc) {

		DxSystemException(DxSystemException::OM_NEW_ERROR).ShowOriginalMessage();

		return false;
	}

	return true;
}

void Game::Release()
{
	if (m_pNowScene) {
		delete m_pNowScene;
		m_pNowScene = nullptr;
	}
}








