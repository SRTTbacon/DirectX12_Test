#pragma once
#include <Windows.h>

constexpr const wchar_t* WINDOW_NAME = L"DirectX12�e�X�g";

const UINT WINDOW_WIDTH = 1920;
const UINT WINDOW_HEIGHT = 1018;

enum WindowMode
{
	WindowNormal,		//�ʏ�̃E�B���h�E
	WindowMaximum,		//�ő剻���ꂽ�E�B���h�E
	WindowBorderLess,	//�{�[�_�[���X�E�B���h�E (�t���X�N���[��)
	FullScreen			//�t���X�N���[��
};

int StartApp(const TCHAR* appName);