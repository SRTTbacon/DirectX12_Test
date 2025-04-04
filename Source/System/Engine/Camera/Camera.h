#pragma once
#include <DirectXMath.h>
#include "..\\..\\Main\\Main.h"
#include "..\\Lights\\DirectionalLight.h"

struct Camera
{
	DirectX::XMVECTOR m_eyePos;
	DirectX::XMVECTOR m_targetPos;
	DirectX::XMVECTOR m_upFoward;

	DirectX::XMMATRIX m_viewMatrix;
	DirectX::XMMATRIX m_projMatrix;

	float m_fov;
	float m_aspect;
	float m_near;
	float m_far;

	float m_yaw;
	float m_pitch;

	void LateUpdate(DirectionalLight* pDirectionalLight);
	void SetFov(float fovDegree);

	Camera();
};
