#include "Camera.h"

using namespace DirectX;

void Camera::SetFov(float fovDegree)
{
	m_fov = XMConvertToRadians(fovDegree);
}

void Camera::Update()
{
    // カメラの前方ベクトルを計算
    XMVECTOR forward = XMVector3Normalize(m_targetPos - m_eyePos);

    // カメラの右方向ベクトルを計算
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(m_upFoward, forward));

    const float cameraSpeed = 0.01f;
    const float rotationSpeed = 0.01f;

    if (g_Engine->GetKeyState(DIK_SPACE)) {
        m_eyePos = XMVectorAdd(m_eyePos, XMVectorScale(m_upFoward, cameraSpeed));
        m_targetPos = XMVectorAdd(m_targetPos, XMVectorScale(m_upFoward, cameraSpeed));
    }
    else if (g_Engine->GetKeyState(DIK_LSHIFT)) {
        m_eyePos = XMVectorSubtract(m_eyePos, XMVectorScale(m_upFoward, cameraSpeed));
        m_targetPos = XMVectorSubtract(m_targetPos, XMVectorScale(m_upFoward, cameraSpeed));
    }

    //Move Right and Left
    if (g_Engine->GetKeyState(DIK_A)) {
        m_eyePos = XMVectorAdd(m_eyePos, XMVectorScale(right, cameraSpeed));
        m_targetPos = XMVectorAdd(m_targetPos, XMVectorScale(right, cameraSpeed));
    }
    else if (g_Engine->GetKeyState(DIK_D)) {
        m_eyePos = XMVectorSubtract(m_eyePos, XMVectorScale(right, cameraSpeed));
        m_targetPos = XMVectorSubtract(m_targetPos, XMVectorScale(right, cameraSpeed));
    }

    //Move forward/Backward
    if (g_Engine->GetKeyState(DIK_S)) {
        m_eyePos = XMVectorSubtract(m_eyePos, XMVectorScale(forward, cameraSpeed));
        m_targetPos = XMVectorSubtract(m_targetPos, XMVectorScale(forward, cameraSpeed));
    }
    else if (g_Engine->GetKeyState(DIK_W)) {
        m_eyePos = XMVectorAdd(m_eyePos, XMVectorScale(forward, cameraSpeed));
        m_targetPos = XMVectorAdd(m_targetPos, XMVectorScale(forward, cameraSpeed));
    }

    //Rotate Y axis
    if (g_Engine->GetKeyState(DIK_RIGHT)) {
        m_yaw -= rotationSpeed;
    }
    if (g_Engine->GetKeyState(DIK_LEFT)) {
        m_yaw += rotationSpeed;
    }

    //Rotate X axis
    if (g_Engine->GetKeyState(DIK_DOWN)) {
        m_pitch -= rotationSpeed;
    }
    if (g_Engine->GetKeyState(DIK_UP)) {
        m_pitch += rotationSpeed;
    }

    m_pitch = max(-XM_PIDIV2 + 0.01f, min(XM_PIDIV2 - 0.01f, m_pitch));

    XMVECTOR direction{};
    direction.m128_f32[0] = cosf(m_pitch) * sinf(m_yaw);
    direction.m128_f32[1] = sinf(m_pitch);
    direction.m128_f32[2] = cosf(m_pitch) * cosf(m_yaw);

    m_targetPos = XMVectorAdd(m_eyePos, XMVector3Normalize(direction));
}

Camera::Camera()
	: m_eyePos(XMVectorSet(0.0f, 1.0f, -3.0f, 0.0f))
	, m_targetPos(XMVectorSet(0.0f, 0.0f, 0.75f, 0.0f))
	, m_upFoward(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f))
	, m_fov(XMConvertToRadians(35.0f))
    , m_aspect(static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT))
    , m_yaw(0.0f)
    , m_pitch(0.0f)
{
}
