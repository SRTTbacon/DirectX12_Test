#include "KinematicMotionState.h"

using namespace DirectX;

KinematicMotionState::KinematicMotionState(Bone* pBoneNode, const DirectX::XMMATRIX& offset)
	: m_pBoneNode(pBoneNode)
	, m_offset(offset)
{
}

void KinematicMotionState::getWorldTransform(btTransform& worldTrans) const
{
	XMMATRIX matrix;
	if (m_pBoneNode != nullptr)
	{
		matrix = m_offset * m_pBoneNode->GetGlobalTransform();
	}
	else
	{
		matrix = m_offset;
	}

	float m[16];
	GetTransposeMatrix(matrix, m);
	worldTrans.setFromOpenGLMatrix(m);
}

void KinematicMotionState::setWorldTransform(const btTransform& worldTrans)
{
}

void KinematicMotionState::Reset()
{
}

void KinematicMotionState::ReflectGlobalTransform()
{
}
