#include "Main.h"

#include "..\\..\\Game.h"

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

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
	auto style = WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU;
	AdjustWindowRect(&rect, style, FALSE);

	//モニターの解像度を取得
	HMONITOR hMonitor = MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTONEAREST);
	MONITORINFO monitorInfo = {};
	monitorInfo.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(hMonitor, &monitorInfo);

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

static int MainLoop()
{
	MSG msg = {};

	LARGE_INTEGER frequency;	//パフォーマンス周波数
	LARGE_INTEGER prevTime;		//前フレームの時間
	LARGE_INTEGER currentTime;	//現在の時間

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&prevTime);

	//ゲームクラス
	Game clsGame;

	try {
		//ゲームクラスの初期化
		try {
			clsGame.Initialize(g_hWnd);
		}
		catch (DxSystemException dxSystemException) {
			dxSystemException.ShowOriginalMessage();
			throw DxSystemException(DxSystemException::OM_GAME_INITIALIZE_ERROR);
		}

	}
	catch (DxSystemException dxSystemException) {

		dxSystemException.ShowOriginalMessage();

		return 1;
	}
	catch (...) {

		DxSystemException(DxSystemException::OM_UNKNOWN_ERROR).ShowOriginalMessage();

		return 1;
	}

	for (;;)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			//終了処理(QUITﾒｯｾｰｼﾞで終了)
			if (msg.message == WM_QUIT) {
				clsGame.Release();

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

			if (!clsGame.Run()) {
				DestroyWindow(g_hWnd);
			}
		}
	}

	return 0;
}

int StartApp(const TCHAR* appName)
{
	//dllの参照場所を指定
	if (!SetDllDirectory(TEXT("Resource\\dll"))) {
		printf("dllディレクトリの指定に失敗しました。");
		return 1;
	}
	//LoadLibrary(L"WinPixGpuCapturer.dll");

	//ウィンドウ生成
	InitWindow(appName);

	//メイン処理ループ
	int r = MainLoop();

#ifdef _DEBUG
	ComPtr<IDXGIDebug1> dxgiDebug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
	{
		dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
	}
#endif

	return r;
}

int main(int argc, wchar_t** argv, wchar_t** envp)
{
	return StartApp(WINDOW_NAME);
}