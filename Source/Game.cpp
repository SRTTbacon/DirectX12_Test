#include "Game.h"

using namespace Common;

//=============================================================================
// コンストラクタ
//=============================================================================
Game::Game()
	: m_pNowScene(nullptr)
{
}

//=============================================================================
// デストラクタ
//=============================================================================
Game::~Game()
{
	//現在のシーンクラスの解放
	if (m_pNowScene) {
		delete m_pNowScene;
		m_pNowScene = nullptr;
	}

	//シーンのスタック解放
	if (!Scene::m_stkScene.empty()) {

		while (!Scene::m_stkScene.empty()) {
			delete Scene::m_stkScene.top().m_pPrevScene;
			Scene::m_stkScene.top().m_pPrevScene = nullptr;
			Scene::m_stkScene.pop();
		}
	}

	//エンジンクラスの解放
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
// ゲームの初期化
// 引　数：const HWND & ウィンドウハンドル
// 　　　　const HINSTANCE & インスタンスハンドル
//=============================================================================
void Game::Initialize(const HWND& hAppWnd)
{
	srand((unsigned int)time(nullptr));

	try {

		//エンジンクラスの初期化
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

	//最初のシーン設定

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
// シーン切り替え
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
// 実行
//=============================================================================
bool Game::Run()
{
	try {

		//シーンが変わったら
		if (Scene::m_nowSceneData.m_scene != Scene::m_prevSceneData.m_scene) {

			//SCENE_EXITで終了
			if (!ChangeScene()) {
				return false;
			}

			Scene::m_prevSceneData = Scene::m_nowSceneData;

			SetCursor(LoadCursor(NULL, IDC_ARROW));
		}

		//Runの戻り値がtrue（終了）　→　シーンが変わった
		if (m_pNowScene->Run()) {

			//スタックを調べる
			if (Scene::m_stkScene.empty()) {

				//スタックが空（現在、サブシーンではない）

				//新しいシーンがサブシーンで無ければ
				if (!Scene::m_nowSceneData.m_bSubScene) {

					//シーンの解放
					delete m_pNowScene;
					m_pNowScene = nullptr;

				}
				else {

					//新しいシーンがサブシーンの場合

					//現在のシーンを保存
					Scene::m_prevSceneData.m_pPrevScene = m_pNowScene;

					//スタックにプッシュ
					Scene::m_stkScene.push(Scene::m_prevSceneData);
				}
			}
			else {

				//スタックにシーン情報がある（現在サブシーンに入っている）

				//スタックのトップ同じシーン　→　シーンを戻す
				if (Scene::m_nowSceneData.m_scene == Scene::m_stkScene.top().m_scene) {

					//シーンの解放
					delete m_pNowScene;
					m_pNowScene = nullptr;

					//保存しておいた前のシーンを戻す
					m_pNowScene = Scene::m_stkScene.top().m_pPrevScene;

					//スタックから前のシーンを消す
					Scene::m_stkScene.pop();

					//サブシーンから戻った事をクラスに伝える
					m_pNowScene->ReturnSubScene();

					Scene::m_prevSceneData = Scene::m_nowSceneData;
				}
				else {

					//スタックのトップと違うシーン

					//SCENE_EXITであれば終了
					if (Scene::m_nowSceneData.m_scene == SCENE_EXIT) {
						return false;
					}

					//サブシーン以外への遷移
					if (!Scene::m_nowSceneData.m_bSubScene) {

						//現在のシーンを解放
						delete m_pNowScene;
						m_pNowScene = nullptr;

						//スタックにあるシーンを全て削除する
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

						//サブシーンへ

						//現在のシーンを保存
						Scene::m_prevSceneData.m_pPrevScene = m_pNowScene;

						//スタックにプッシュ
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








