#include "DynamicMotionState.h"

using namespace DirectX;

DynamicMotionState::DynamicMotionState(Bone* pBoneNode, const DirectX::XMMATRIX& offset, bool override)
	: m_pBoneNode(pBoneNode)
	, m_offset(offset)
	, m_bOverride(override)
{
	XMVECTOR determinant = XMMatrixDeterminant(m_offset);
	m_invOffset = XMMatrixInverse(&determinant, m_offset);

	Reset();
}

void DynamicMotionState::getWorldTransform(btTransform& worldTransform) const
{
	worldTransform = m_transform;
}

void DynamicMotionState::setWorldTransform(const btTransform& worldTrans)
{
	m_transform = worldTrans;
}

void DynamicMotionState::Reset()
{
	XMMATRIX global = m_offset * m_pBoneNode->GetGlobalTransform();
	float m[16];
	GetTransposeMatrix(global, m);
	m_transform.setFromOpenGLMatrix(m);
}

void DynamicMotionState::ReflectGlobalTransform()
{
	XMMATRIX world = GetMatrixFrombtTransform(m_transform);
	XMMATRIX result = m_invOffset * world;

	if (m_bOverride == true)
	{
		m_pBoneNode->SetGlobalTransform(result);
		m_pBoneNode->UpdateChildTransform();
	}
}
