#include "LightTransform.h"

using namespace DirectX;

LightTransform::LightTransform() :
	m_position(XMFLOAT3(0.0f, 0.0f, 0.0f)),
	m_rotation(XMFLOAT3(0.0f, 0.0f, 0.0f)),
	m_scale(XMFLOAT3(1.0f, 1.0f, 1.0f))
{
}

void LightTransform::SetRotation(float x, float y, float z)
{
	m_rotation.x = XMConvertToRadians(x);
	m_rotation.y = XMConvertToRadians(y);
	m_rotation.z = XMConvertToRadians(z);
}

const XMFLOAT3 LightTransform::GetRotation() const
{
	return XMFLOAT3(XMConvertToDegrees(m_rotation.x), XMConvertToDegrees(m_rotation.y), XMConvertToDegrees(m_rotation.z));
}

const XMFLOAT4 LightTransform::GetQuaternion() const
{
	XMFLOAT4 result{};
	XMVECTOR quaternion = XMQuaternionRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
	XMStoreFloat4(&result, quaternion);
	return result;
}

void LightTransform::AddTranslation(float x, float y, float z)
{
	m_position.x += x;
	m_position.y += y;
	m_position.z += z;
}

void LightTransform::AddRotationX(float x)
{
	m_rotation.x += XMConvertToRadians(x);
}
void LightTransform::AddRotationY(float y)
{
	m_rotation.y += XMConvertToRadians(y);
}
void LightTransform::AddRotationZ(float z)
{
	m_rotation.z += XMConvertToRadians(z);
}

XMFLOAT3 LightTransform::GetForward() const
{
	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
	XMFLOAT3 result = XMFLOAT3(rotation.r[2].m128_f32[0], rotation.r[2].m128_f32[1], rotation.r[2].m128_f32[2]);
	return result;
}

XMFLOAT3 LightTransform::GetRight() const
{
	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
	XMFLOAT3 result = XMFLOAT3(rotation.r[0].m128_f32[0], rotation.r[0].m128_f32[1], rotation.r[0].m128_f32[2]);
	return result;
}

XMFLOAT3 LightTransform::GetUp() const
{
	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
	XMFLOAT3 result = XMFLOAT3(rotation.r[1].m128_f32[0], rotation.r[1].m128_f32[1], rotation.r[1].m128_f32[2]);
	return result;
}

XMMATRIX LightTransform::GetTransformMatrix() const
{
	XMMATRIX s = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
	XMMATRIX r = XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
	XMMATRIX t = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	XMMATRIX result = s * r * t;

	return result;
}

XMMATRIX LightTransform::GetViewMatrix() const
{
	XMFLOAT3 forward = GetForward();
	XMFLOAT3 up = GetUp();

	XMVECTOR positionVector = XMLoadFloat3(&m_position);
	XMVECTOR forwardVector = XMLoadFloat3(&forward);
	XMVECTOR upVector = XMLoadFloat3(&up);

	XMMATRIX result = XMMatrixLookToLH(positionVector, forwardVector, upVector);
	return result;
}