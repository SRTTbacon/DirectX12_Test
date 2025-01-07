#pragma once

#include <btBulletDynamicsCommon.h>
#include <DirectXMath.h>

static void GetTransposeMatrix(DirectX::XMMATRIX& matrix, float* out)
{
	DirectX::XMMATRIX& m = matrix;

	out[0] = m.r[0].m128_f32[0];
	out[1] = m.r[1].m128_f32[0];
	out[2] = m.r[2].m128_f32[0];
	out[3] = m.r[3].m128_f32[0];
	out[4] = m.r[0].m128_f32[1];
	out[5] = m.r[1].m128_f32[1];
	out[6] = m.r[2].m128_f32[1];
	out[7] = m.r[3].m128_f32[1];
	out[8] = m.r[0].m128_f32[2];
	out[9] = m.r[1].m128_f32[2];
	out[10] = m.r[2].m128_f32[2];
	out[11] = m.r[3].m128_f32[2];
	out[12] = m.r[0].m128_f32[3];
	out[13] = m.r[1].m128_f32[3];
	out[14] = m.r[2].m128_f32[3];
	out[15] = m.r[3].m128_f32[3];
}

static DirectX::XMMATRIX GetMatrixFrombtTransform(btTransform& btTransform)
{
	float m[16];
	btTransform.getOpenGLMatrix(m);

	DirectX::XMMATRIX resultMatrix{};
	resultMatrix.r[0].m128_f32[0] = m[0];
	resultMatrix.r[1].m128_f32[0] = m[1];
	resultMatrix.r[2].m128_f32[0] = m[2];
	resultMatrix.r[3].m128_f32[0] = m[3];

	resultMatrix.r[0].m128_f32[1] = m[4];
	resultMatrix.r[1].m128_f32[1] = m[5];
	resultMatrix.r[2].m128_f32[1] = m[6];
	resultMatrix.r[3].m128_f32[1] = m[7];

	resultMatrix.r[0].m128_f32[2] = m[8];
	resultMatrix.r[1].m128_f32[2] = m[9];
	resultMatrix.r[2].m128_f32[2] = m[10];
	resultMatrix.r[3].m128_f32[2] = m[11];

	resultMatrix.r[0].m128_f32[3] = m[12];
	resultMatrix.r[1].m128_f32[3] = m[13];
	resultMatrix.r[2].m128_f32[3] = m[14];
	resultMatrix.r[3].m128_f32[3] = m[15];

	resultMatrix = XMMatrixTranspose(resultMatrix);

	return resultMatrix;
}

class MotionState : public btMotionState
{
public:
	virtual void Reset() = 0;
	virtual void ReflectGlobalTransform() = 0;
};
