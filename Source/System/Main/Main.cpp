#include "Main.h"

#include "..\\..\\Game.h"

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

HINSTANCE g_hInst;
HWND g_hWnd = NULL;

//FPS�v�Z�p�̕ϐ�
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

	//�E�B���h�E�̐ݒ�
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

	//�E�B���h�E�N���X�̓o�^�B
	RegisterClassEx(&wc);

	//�E�B���h�E�T�C�Y�̐ݒ�
	RECT rect = {};
	rect.right = static_cast<LONG>(WINDOW_WIDTH);
	rect.bottom = static_cast<LONG>(WINDOW_HEIGHT);

	//�E�B���h�E�T�C�Y�𒲐�
	auto style = WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU;
	AdjustWindowRect(&rect, style, FALSE);

	//���j�^�[�̉𑜓x���擾
	HMONITOR hMonitor = MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTONEAREST);
	MONITORINFO monitorInfo = {};
	monitorInfo.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(hMonitor, &monitorInfo);

	//�E�B���h�E�̐���
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

	//�E�B���h�E��\��
	ShowWindow(g_hWnd, SW_SHOW);

	//�E�B���h�E�Ƀt�H�[�J�X����
	SetFocus(g_hWnd);
}

static int MainLoop()
{
	MSG msg = {};

	LARGE_INTEGER frequency;	//�p�t�H�[�}���X���g��
	LARGE_INTEGER prevTime;		//�O�t���[���̎���
	LARGE_INTEGER currentTime;	//���݂̎���

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&prevTime);

	//�Q�[���N���X
	Game clsGame;

	try {
		//�Q�[���N���X�̏�����
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
			//�I������(QUITү���ނŏI��)
			if (msg.message == WM_QUIT) {
				clsGame.Release();

				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			//���݂̎��Ԃ��擾
			QueryPerformanceCounter(&currentTime);
			float deltaTime = static_cast<float>(currentTime.QuadPart - prevTime.QuadPart) / frequency.QuadPart;
			prevTime = currentTime;

			//�o�ߎ��Ԃ����Z
			elapsedTime += deltaTime;
			frameCount++;

			//1�b�𒴂�����FPS���v�Z
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
	//dll�̎Q�Əꏊ���w��
	if (!SetDllDirectory(TEXT("Resource\\dll"))) {
		printf("dll�f�B���N�g���̎w��Ɏ��s���܂����B");
		return 1;
	}
	//LoadLibrary(L"WinPixGpuCapturer.dll");

	//�E�B���h�E����
	InitWindow(appName);

	//���C���������[�v
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