#pragma once
#include <Windows.h>

constexpr const wchar_t* WINDOW_NAME = L"DirectX12テスト";

const UINT WINDOW_WIDTH = 1920;
const UINT WINDOW_HEIGHT = 1018;

enum WindowMode
{
	WindowNormal,		//通常のウィンドウ
	WindowMaximum,		//最大化されたウィンドウ
	WindowBorderLess,	//ボーダーレスウィンドウ (フルスクリーン)
	FullScreen			//フルスクリーン
};

int StartApp(const TCHAR* appName);