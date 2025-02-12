#pragma once
#include <DirectXMath.h>
#include "..\\..\\Main\\Main.h"
#include "..\\Lights\\DirectionalLight.h"

struct Camera
{
	DirectX::XMVECTOR m_eyePos;
	DirectX::XMVECTOR m_targetPos;
	DirectX::XMVECTOR m_upFoward;

	float m_fov;
	float m_aspect;
	float m_near;
	float m_far;

	float m_yaw;
	float m_pitch;

	float m_test;

	void Update(DirectionalLight* pDirectionalLight);
	void SetFov(float fovDegree);

	Camera();
};
