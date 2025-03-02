#include "Main.h"
#include "..\\Engine\\Engine.h"
#include "..\\..\\Scene\\Scene\\Scene.h"

HINSTANCE g_hInst;
HWND g_hWnd = NULL;

//FPS計算用の変数
static int frameCount = 0;
static float elapsedTime = 0.0f;

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		break;
	}

	return DefWindowProc(hWnd, msg, wp, lp);
}

static void InitWindow(const TCHAR* appName)
{
	g_hInst = GetModuleHandle(nullptr);
	if (g_hInst == nullptr)
	{
		return;
	}

	//ウィンドウの設定
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hIcon = LoadIcon(g_hInst, IDI_APPLICATION);
	wc.hCursor = LoadCursor(g_hInst, IDC_ARROW);
	wc.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = appName;
	wc.hIconSm = LoadIcon(g_hInst, IDI_APPLICATION);

	//ウィンドウクラスの登録。
	RegisterClassEx(&wc);

	//ウィンドウサイズの設定
	RECT rect = {};
	rect.right = static_cast<LONG>(WINDOW_WIDTH);
	rect.bottom = static_cast<LONG>(WINDOW_HEIGHT);

	//ウィンドウサイズを調整
	auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	AdjustWindowRect(&rect, style, FALSE);

	//ウィンドウの生成
	g_hWnd = CreateWindowEx(
		0,
		appName,
		appName,
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		nullptr,
		nullptr,
		g_hInst,
		nullptr
	);

	//ウィンドウを表示
	ShowWindow(g_hWnd, SW_SHOW);

	//ウィンドウにフォーカスする
	SetFocus(g_hWnd);
}

static void MainLoop()
{
	MSG msg = {};

	LARGE_INTEGER frequency;	//パフォーマンス周波数
	LARGE_INTEGER prevTime;		//前フレームの時間
	LARGE_INTEGER currentTime;	//現在の時間

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&prevTime);

	for (;;)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			//終了処理(QUITﾒｯｾｰｼﾞで終了)
			if (msg.message == WM_QUIT) {
				delete g_Scene;
				delete g_Engine;
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			//現在の時間を取得
			QueryPerformanceCounter(&currentTime);
			float deltaTime = static_cast<float>(currentTime.QuadPart - prevTime.QuadPart) / frequency.QuadPart;
			prevTime = currentTime;

			//経過時間を加算
			elapsedTime += deltaTime;
			frameCount++;

			//1秒を超えたらFPSを計算
			if (elapsedTime >= 1.0f)
			{
				int fps = frameCount;
				printf("%d[FPS]\n", fps);
				frameCount = 0;
				elapsedTime = 0.0f;
			}

			g_Engine->Update();
			g_Scene->Update();
			g_Engine->LateUpdate();
			g_Engine->BeginRender();
			g_Scene->Draw();
			g_Engine->EndRender();
		}
	}

	//終了時
	if (g_Scene) {
		//delete g_Scene;
		g_Scene = nullptr;
	}
	if (g_Engine) {
		//delete g_Engine;
		g_Engine = nullptr;
	}
}

void StartApp(const TCHAR* appName)
{
	//dllの参照場所を指定
	if (!SetDllDirectory(TEXT("Resource\\dll"))) {
		printf("dllディレクトリの指定に失敗しました。");
		return;
	}
	//LoadLibrary(L"WinPixGpuCapturer.dll");

	//ウィンドウ生成
	InitWindow(appName);

	//描画エンジンの初期化を行う
	g_Engine = new Engine(g_hWnd);
	if (!g_Engine->Init(WINDOW_WIDTH, WINDOW_HEIGHT))
	{
		return;
	}

	//シーン初期化
	g_Scene = new Scene();
	if (!g_Scene->Init())
	{
		return;
	}

	//メイン処理ループ
	MainLoop();
}


int main(int argc, wchar_t** argv, wchar_t** envp)
{
	StartApp(TEXT("DirectX12テスト"));
	return 0;
}