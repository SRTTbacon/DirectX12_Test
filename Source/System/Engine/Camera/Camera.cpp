#include "Camera.h"

using namespace DirectX;

void Camera::SetFov(float fovDegree)
{
	m_fov = XMConvertToRadians(fovDegree);
	m_constantBuffer->Proj = XMMatrixPerspectiveFovRH(m_fov, 1920.0f / 1080.0f, 0.1f, 1000.0f);
}

void Camera::Update()
{
	/*if (g_Engine->GetKeyState(DIK_LEFT)) {
		m_eyePos.x -= 0.1f;
	}
	if (g_Engine->GetKeyState(DIK_RIGHT)) {
		m_eyePos.x += 0.1f;
	}
	XMVECTOR eyePos = XMVectorSet(m_eyePos.x, m_eyePos.y, m_eyePos.z, 0.0f);
	XMVECTOR targetPos = XMVectorSet(m_targetPos.x, m_targetPos.y, m_targetPos.z, 0.0f);
	XMVECTOR upFoward = XMVectorSet(m_upFoward.x, m_upFoward.y, m_upFoward.z, 0.0f);
	m_constantBuffer->View = XMMatrixLookAtRH(eyePos, targetPos, upFoward);*/
    // Get Datas
    XMMATRIX& viewMatrix = m_constantBuffer->View;

    //Move Up and bottom
    if (g_Engine->GetKeyState(DIK_SPACE)) {
        viewMatrix *= XMMatrixTranslation(0.0f, -0.1f, 0.0f);
    }
    else if (g_Engine->GetKeyState(DIK_LSHIFT)) {
        viewMatrix *= XMMatrixTranslation(0.0f, 0.1f, 0.0f);
    }

    //Move Right and Left
    if (g_Engine->GetKeyState(DIK_A)) {
        viewMatrix *= XMMatrixTranslation(0.1f, 0.0f, 0.0f);
    }
    else if (g_Engine->GetKeyState(DIK_D)) {
        viewMatrix *= XMMatrixTranslation(-0.1f, 0.0f, 0.0f);
    }

    //Move forward/Backward
    if (g_Engine->GetKeyState(DIK_S)) {
        viewMatrix *= XMMatrixTranslation(0.0f, 0.0f, -0.1f);
    }
    else if (g_Engine->GetKeyState(DIK_W)) {
        viewMatrix *= XMMatrixTranslation(0.0f, 0.0f, 0.1f);
    }

    //Rotate Y axis
    if (g_Engine->GetKeyState(DIK_RIGHT)) {
        viewMatrix *= XMMatrixRotationY(0.01f);
    }
    if (g_Engine->GetKeyState(DIK_LEFT)) {
        viewMatrix *= XMMatrixRotationY(-0.01f);
    }

    //Rotate X axis
    if (g_Engine->GetKeyState(DIK_DOWN)) {
        viewMatrix *= XMMatrixRotationX(0.01f);
    }
    if (g_Engine->GetKeyState(DIK_UP)) {
        viewMatrix *= XMMatrixRotationX(-0.01f);
    }

    //Rotate Z axis
    if (g_Engine->GetKeyState(DIK_X)) {
        viewMatrix *= XMMatrixRotationZ(0.01f);
    }
    if (g_Engine->GetKeyState(DIK_Z)) {
        viewMatrix *= XMMatrixRotationZ(-0.01f);
    }
}

Camera::Camera()
	: m_eyePos(XMFLOAT3(0.0f, 20.0f, 25.0f))
	, m_targetPos(XMFLOAT3(0.0f, 8.0f, 0.0f))
	, m_upFoward(XMFLOAT3(0.0f, 1.0f, 0.0f))
	, m_fov(XMConvertToRadians(40.0f))
{
	m_constantBuffer = new Transform();
	// •ÏŠ·s—ñ‚Ì“o˜^
	m_constantBuffer->World = XMMatrixIdentity();
	m_constantBuffer->Proj = XMMatrixPerspectiveFovRH(m_fov, 1920.0f / 1080.0f, 0.1f, 1000.0f);
    m_constantBuffer->View = XMMatrixLookAtRH(XMVectorSet(m_eyePos.x, m_eyePos.y, m_eyePos.z, 0.0f),
        XMVectorSet(m_targetPos.x, m_targetPos.y, m_targetPos.z, 0.0f), XMVectorSet(m_upFoward.x, m_upFoward.y, m_upFoward.z, 0.0f));
}
