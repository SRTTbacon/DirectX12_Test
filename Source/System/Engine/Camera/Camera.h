#pragma once
#include "..\\Engine.h"
#include "..\\Core\\ConstantBuffer\\ConstantBuffer.h"
#include <DirectXMath.h>
#include "..\\Core\\SharedStruct\\SharedStruct.h"

struct Camera
{
	Transform* m_constantBuffer;

	DirectX::XMFLOAT3 m_eyePos;
	DirectX::XMFLOAT3 m_targetPos;
	DirectX::XMFLOAT3 m_upFoward;

	float m_fov;

	void SetFov(float fovDegree);

	void Update();

	Camera();
};