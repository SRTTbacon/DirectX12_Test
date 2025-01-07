#pragma once

#include "MotionState.h"

class DefaultMotionState : public MotionState
{
public:
	DefaultMotionState(const DirectX::XMMATRIX& transform)
	{
		float m[16];
		GetTransposeMatrix(transform, m);
		m_transform.setFromOpenGLMatrix(m);
		m_initTransform = m_transform;
	}

	void getWorldTransform(btTransform& worldTrans) const override
	{
		worldTrans = m_transform;
	}

	void setWorldTransform(const btTransform& worldTrans) override
	{
		m_transform = worldTrans;
	}

	void Reset() override
	{

	}

	void ReflectGlobalTransform() override
	{

	}

private:
	btTransform m_initTransform;
	btTransform m_transform;
};
