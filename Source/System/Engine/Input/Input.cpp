#include "Input.h"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

//����}�N��
#define Release(X) { if((X) != nullptr) (X)->Release(); (X) = nullptr; }

//�R���X�g���N�^
Input::Input(HWND win)
    : m_hwnd(win)
    , result(S_OK)
    , input(nullptr)
    , key(nullptr)
    , m_bCanResponseUnFocus(false)
{
    memset(&keys, 0, sizeof(keys));
    memset(&olds, 0, sizeof(olds));

    CreateInput();
    CreateKey();
    SetKeyFormat();
    SetKeyCooperative();
    CreateMouseState();
}

//�f�X�g���N�^
Input::~Input()
{
    Release(key);
    Release(input);
}

//�C���v�b�g�̐���
HRESULT Input::CreateInput()
{
    result = DirectInput8Create(GetModuleHandle(0), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)(&input), NULL);

    return result;
}
//�L�[�f�o�C�X�̐���

HRESULT Input::CreateKey()
{
    result = input->CreateDevice(GUID_SysKeyboard, &key, NULL);

    return result;
}

//�L�[�t�H�[�}�b�g�̃Z�b�g
HRESULT Input::SetKeyFormat()
{
    result = key->SetDataFormat(&c_dfDIKeyboard);

    return result;
}

//�L�[�̋������x���̃Z�b�g
HRESULT Input::SetKeyCooperative()
{
    result = key->SetCooperativeLevel(m_hwnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);

    //���̓f�o�C�X�ւ̃A�N�Z�X�������擾
    key->Acquire();

    return result;
}

bool Input::CreateMouseState()
{
    //�}�E�X�f�o�C�X�̍쐬
    if (FAILED(input->CreateDevice(GUID_SysMouse, &m_pMouseDevice, nullptr))) {
        return false;
    }

    //�f�[�^�t�H�[�}�b�g�̎w��
    if (FAILED(m_pMouseDevice->SetDataFormat(&c_dfDIMouse))) {
        return false;
    }

    //�������x���̐ݒ�i�A�N�e�B�u����r���j
    if (FAILED(m_pMouseDevice->SetCooperativeLevel(m_hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE))) {
        return false;
    }

    //�v���p�e�B�̐ݒ�
    DIPROPDWORD diProperty{};

    diProperty.diph.dwSize = sizeof(diProperty);
    diProperty.diph.dwHeaderSize = sizeof(diProperty.diph);
    diProperty.diph.dwObj = 0;
    diProperty.diph.dwHow = DIPH_DEVICE;
    diProperty.dwData = DIPROPAXISMODE_REL;

    if (FAILED(m_pMouseDevice->SetProperty(DIPROP_AXISMODE, &diProperty.diph))) {
        return false;
    }

    //�A�N�Z�X���̎擾
    m_pMouseDevice->Acquire();

    //�`�F�b�N�p�̈�̃N���A
    memset(m_mouseCheck.rgbButtons, 0, sizeof(BYTE) * (sizeof(m_mouseCheck.rgbButtons) / sizeof(m_mouseCheck.rgbButtons[0])));

    return true;
}

//�L�[����
bool Input::CheckKey(UINT index)
{
    //�E�B���h�E�Ƀt�H�[�J�X���Ȃ��ꍇ�̓L�[������󂯕t���Ȃ�
    if (!m_bCanResponseUnFocus && GetForegroundWindow() != m_hwnd) {
        return false;
    }

    //�`�F�b�N�t���O
    bool flag = false;
    //�L�[�����擾
    key->GetDeviceState(sizeof(keys), &keys);
    if (keys[index] & 0x80)
    {
        flag = true;
    }
    olds[index] = keys[index];

    return flag;
}

//�g���K�[�̓���
bool Input::TriggerKey(UINT index)
{
    //�E�B���h�E�Ƀt�H�[�J�X���Ȃ��ꍇ�̓L�[������󂯕t���Ȃ�
    if (!m_bCanResponseUnFocus && GetForegroundWindow() != m_hwnd) {
        return false;
    }

    //�`�F�b�N�t���O
    bool flag = false;

    //�L�[�����擾
    key->GetDeviceState(sizeof(keys), &keys);
    if ((keys[index] & 0x80) && !(olds[index] & 0x80))
    {
        flag = true;
    }
    olds[index] = keys[index];

    return flag;
}

void Input::UpdateMouseState()
{
    if (FAILED(m_pMouseDevice->GetDeviceState(sizeof(DIMOUSESTATE), &m_mouseState))) {
        m_pMouseDevice->Acquire();
    }
}

POINT Input::GetMouseMove() const
{
    POINT point{};

    point.x = m_mouseState.lX;
    point.y = m_mouseState.lY;

    return point;
}

int Input::GetMouseWheelDelta() const
{
    return m_mouseState.lZ;
}

BYTE Input::GetMouseState(const BYTE keyCode) const
{
    return m_mouseState.rgbButtons[keyCode];
}

BYTE Input::GetMouseStateSync(const BYTE keyCode)
{
    if (m_mouseState.rgbButtons[keyCode]) {
        if (m_mouseCheck.rgbButtons[keyCode] == 0x00) {
            m_mouseCheck.rgbButtons[keyCode] = m_mouseState.rgbButtons[keyCode];
            return m_mouseState.rgbButtons[keyCode];
        }
        else {
            return 0x00;
        }
    }
    else {
        if (m_mouseCheck.rgbButtons[keyCode]) {
            m_mouseCheck.rgbButtons[keyCode] = m_mouseState.rgbButtons[keyCode];
        }
        return 0x00;
    }
}
