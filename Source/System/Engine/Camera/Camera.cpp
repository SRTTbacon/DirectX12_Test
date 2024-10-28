#include "Camera.h"

using namespace DirectX;

void Camera::SetFov(float fovDegree)
{
	m_fov = XMConvertToRadians(fovDegree);
}

void Camera::Update()
{
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
