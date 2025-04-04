#include "Transform.h"

using namespace DirectX;

Transform::Transform()
	: m_position(XMFLOAT3(0.0f, 0.0f, 0.0f))
	, m_rotation(XMFLOAT3(0.0f, 0.0f, 0.0f))
	, m_scale(XMFLOAT3(1.0f, 1.0f, 1.0f))
	, m_bVisible(true)
{
}

void Transform::SetRotation(float x, float y, float z)
{
	m_rotation.x = XMConvertToRadians(x);
	m_rotation.y = XMConvertToRadians(y);
	m_rotation.z = XMConvertToRadians(z);
}

const XMFLOAT3 Transform::GetDegreeRotation() const
{
	return XMFLOAT3(XMConvertToDegrees(m_rotation.x), XMConvertToDegrees(m_rotation.y), XMConvertToDegrees(m_rotation.z));
}

const XMFLOAT4 Transform::GetQuaternion() const
{
	XMFLOAT4 result{};
	XMVECTOR quaternion = XMQuaternionRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
	XMStoreFloat4(&result, quaternion);
	return result;
}

void Transform::AddTranslation(float x, float y, float z)
{
	m_position.x += x;
	m_position.y += y;
	m_position.z += z;
}

void Transform::AddRotationX(float x)
{
	m_rotation.x += XMConvertToRadians(x);
}
void Transform::AddRotationY(float y)
{
	m_rotation.y += XMConvertToRadians(y);
}
void Transform::AddRotationZ(float z)
{
	m_rotation.z += XMConvertToRadians(z);
}

XMFLOAT3 Transform::GetForward() const
{
	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
	XMFLOAT3 result = XMFLOAT3(rotation.r[2].m128_f32[0], rotation.r[2].m128_f32[1], rotation.r[2].m128_f32[2]);
	return result;
}

XMFLOAT3 Transform::GetRight() const
{
	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
	XMFLOAT3 result = XMFLOAT3(rotation.r[0].m128_f32[0], rotation.r[0].m128_f32[1], rotation.r[0].m128_f32[2]);
	return result;
}

XMFLOAT3 Transform::GetUp() const
{
	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
	XMFLOAT3 result = XMFLOAT3(rotation.r[1].m128_f32[0], rotation.r[1].m128_f32[1], rotation.r[1].m128_f32[2]);
	return result;
}

XMMATRIX Transform::GetTransformMatrix() const
{
	XMMATRIX s = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
	XMMATRIX r = XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
	XMMATRIX t = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	XMMATRIX result = s * r * t;

	return result;
}

XMMATRIX Transform::GetViewMatrix() const
{
	XMFLOAT3 forward = GetForward();
	XMFLOAT3 up = GetUp();

	XMVECTOR positionVector = XMLoadFloat3(&m_position);
	XMVECTOR forwardVector = XMLoadFloat3(&forward);

	XMVECTOR right = XMVector3Normalize(XMVector3Cross({ 0, 0, 1 }, forwardVector));
	XMVECTOR up2 = XMVector3Cross(forwardVector, right); //åç∑ÇµÇƒíºåÇ∑ÇÈUpÇåvéZ

	XMVECTOR upVector = XMLoadFloat3(&up);

	XMMATRIX result = XMMatrixLookToLH(positionVector, forwardVector, up2);

	return result;
}