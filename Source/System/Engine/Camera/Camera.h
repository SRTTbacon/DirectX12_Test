#pragma once
#include "..\\Engine.h"
#include "..\\..\\Main\\Main.h"

struct Camera
{
	DirectX::XMVECTOR m_eyePos;
	DirectX::XMVECTOR m_targetPos;
	DirectX::XMVECTOR m_upFoward;

	float m_fov;
	float m_aspect;

	float m_yaw;
	float m_pitch;

	void Update();
	void SetFov(float fovDegree);

	Camera();
};
