#include "Input.h"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

//解放マクロ
#define Release(X) { if((X) != nullptr) (X)->Release(); (X) = nullptr; }

//コンストラクタ
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

//デストラクタ
Input::~Input()
{
    Release(key);
    Release(input);
}

//インプットの生成
HRESULT Input::CreateInput()
{
    result = DirectInput8Create(GetModuleHandle(0), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)(&input), NULL);

    return result;
}
//キーデバイスの生成

HRESULT Input::CreateKey()
{
    result = input->CreateDevice(GUID_SysKeyboard, &key, NULL);

    return result;
}

//キーフォーマットのセット
HRESULT Input::SetKeyFormat()
{
    result = key->SetDataFormat(&c_dfDIKeyboard);

    return result;
}

//キーの協調レベルのセット
HRESULT Input::SetKeyCooperative()
{
    result = key->SetCooperativeLevel(m_hwnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);

    //入力デバイスへのアクセス権利を取得
    key->Acquire();

    return result;
}

bool Input::CreateMouseState()
{
    //マウスデバイスの作成
    if (FAILED(input->CreateDevice(GUID_SysMouse, &m_pMouseDevice, nullptr))) {
        return false;
    }

    //データフォーマットの指定
    if (FAILED(m_pMouseDevice->SetDataFormat(&c_dfDIMouse))) {
        return false;
    }

    //協調レベルの設定（アクティブかつ非排他）
    if (FAILED(m_pMouseDevice->SetCooperativeLevel(m_hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE))) {
        return false;
    }

    //プロパティの設定
    DIPROPDWORD diProperty{};

    diProperty.diph.dwSize = sizeof(diProperty);
    diProperty.diph.dwHeaderSize = sizeof(diProperty.diph);
    diProperty.diph.dwObj = 0;
    diProperty.diph.dwHow = DIPH_DEVICE;
    diProperty.dwData = DIPROPAXISMODE_REL;

    if (FAILED(m_pMouseDevice->SetProperty(DIPROP_AXISMODE, &diProperty.diph))) {
        return false;
    }

    //アクセス権の取得
    m_pMouseDevice->Acquire();

    //チェック用領域のクリア
    memset(m_mouseCheck.rgbButtons, 0, sizeof(BYTE) * (sizeof(m_mouseCheck.rgbButtons) / sizeof(m_mouseCheck.rgbButtons[0])));

    return true;
}

//キー入力
bool Input::CheckKey(UINT index)
{
    //ウィンドウにフォーカスがない場合はキー操作を受け付けない
    if (!m_bCanResponseUnFocus && GetForegroundWindow() != m_hwnd) {
        return false;
    }

    //チェックフラグ
    bool flag = false;
    //キー情報を取得
    key->GetDeviceState(sizeof(keys), &keys);
    if (keys[index] & 0x80)
    {
        flag = true;
    }
    olds[index] = keys[index];

    return flag;
}

//トリガーの入力
bool Input::TriggerKey(UINT index)
{
    //ウィンドウにフォーカスがない場合はキー操作を受け付けない
    if (!m_bCanResponseUnFocus && GetForegroundWindow() != m_hwnd) {
        return false;
    }

    //チェックフラグ
    bool flag = false;

    //キー情報を取得
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
