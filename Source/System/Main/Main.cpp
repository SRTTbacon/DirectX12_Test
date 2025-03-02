#include "Main.h"
#include "..\\Engine\\Engine.h"
#include "..\\..\\Scene\\Scene\\Scene.h"

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
	auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	AdjustWindowRect(&rect, style, FALSE);

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

static void MainLoop()
{
	MSG msg = {};

	LARGE_INTEGER frequency;	//�p�t�H�[�}���X���g��
	LARGE_INTEGER prevTime;		//�O�t���[���̎���
	LARGE_INTEGER currentTime;	//���݂̎���

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&prevTime);

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

			g_Engine->Update();
			g_Scene->Update();
			g_Engine->LateUpdate();
			g_Engine->BeginRender();
			g_Scene->Draw();
			g_Engine->EndRender();
		}
	}

	//�I����
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
	//dll�̎Q�Əꏊ���w��
	if (!SetDllDirectory(TEXT("Resource\\dll"))) {
		printf("dll�f�B���N�g���̎w��Ɏ��s���܂����B");
		return;
	}
	//LoadLibrary(L"WinPixGpuCapturer.dll");

	//�E�B���h�E����
	InitWindow(appName);

	//�`��G���W���̏��������s��
	g_Engine = new Engine(g_hWnd);
	if (!g_Engine->Init(WINDOW_WIDTH, WINDOW_HEIGHT))
	{
		return;
	}

	//�V�[��������
	g_Scene = new Scene();
	if (!g_Scene->Init())
	{
		return;
	}

	//���C���������[�v
	MainLoop();
}


int main(int argc, wchar_t** argv, wchar_t** envp)
{
	StartApp(TEXT("DirectX12�e�X�g"));
	return 0;
}