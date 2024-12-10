#include "Main.h"
#include "..\\Engine\\Engine.h"
#include "..\\..\\Scene\\Scene\\Scene.h"

HINSTANCE g_hInst;
HWND g_hWnd = NULL;

LARGE_INTEGER str;
std::list <LONGLONG> strTime;
std::list <LARGE_INTEGER> mTimes;
LONGLONG strTime_sum = 0;
LONGLONG oldCount = 0;
LONGLONG freq = 0;

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
	auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	AdjustWindowRect(&rect, style, FALSE);

	// �E�B���h�E�̐���
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

	// �E�B���h�E��\��
	ShowWindow(g_hWnd, SW_SHOW);

	//�E�B���h�E�Ƀt�H�[�J�X����
	SetFocus(g_hWnd);

	//dll�̎Q�Əꏊ���w��
	if (!SetDllDirectory(TEXT("Resource\\dll"))) {
		printf("dll�f�B���N�g���̎w��Ɏ��s���܂����B");
		return;
	}
}

//#include <time.h>
static double GetFPS()
{
	double fps_result;
	LARGE_INTEGER str;
	QueryPerformanceCounter(&str);
	strTime_sum = (str.QuadPart - oldCount) + strTime_sum - strTime.back();
	strTime.push_front(str.QuadPart - oldCount);
	strTime.pop_back();
	oldCount = str.QuadPart;
	QueryPerformanceFrequency(&str);
	freq = str.QuadPart;
	fps_result = freq * 1000.0 / strTime_sum;
	return (double)fps_result;
}
static void MainLoop()
{
	MSG msg = {};
	for (;;)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			//�I������(QUITү���ނŏI��)
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
			clock_t start = clock();
			g_Engine->Update();
			g_Scene->Update();
			g_Engine->LateUpdate();
			g_Engine->BeginRender();
			g_Scene->Draw();
			g_Engine->EndRender();
			clock_t end = clock();
			const double time = static_cast<double>(end - start) / CLOCKS_PER_SEC * 1000.0;
			//printf("%lf[FPS]\n", time);
		}
	}
}

void StartApp(const TCHAR* appName)
{
	//�E�B���h�E����
	InitWindow(appName);

	//�`��G���W���̏��������s��
	g_Engine = new Engine();
	if (!g_Engine->Init(g_hWnd, WINDOW_WIDTH, WINDOW_HEIGHT))
	{
		return;
	}

	//�V�[��������
	g_Scene = new Scene();
	if (!g_Scene->Init())
	{
		return;
	}

	if (QueryPerformanceCounter(&str))
	{
		oldCount = str.QuadPart;
		std::list <LONGLONG> InitstrTime(1000);
		strTime = InitstrTime;
	}
	else
	{
		return;
	}

	// ���C���������[�v
	MainLoop();
}


int main(int argc, wchar_t** argv, wchar_t** envp)
{
	StartApp(TEXT("DirectX12�e�X�g"));
	return 0;
}