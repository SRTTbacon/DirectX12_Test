#include "DynamicAndBoneMergeMotionState.h"

using namespace DirectX;

DynamicAndBoneMergeMotionState::DynamicAndBoneMergeMotionState(Bone* pBoneNode, const DirectX::XMMATRIX& offset, bool override)
	: m_pBoneNode(pBoneNode)
	, m_offset(offset)
	, m_bOverride(override)
{
	XMVECTOR determinant = XMMatrixDeterminant(m_offset);
	m_invOffset = XMMatrixInverse(&determinant, offset);
	Reset();
}

void DynamicAndBoneMergeMotionState::getWorldTransform(btTransform& worldTrans) const
{
	worldTrans = m_transform;
}

void DynamicAndBoneMergeMotionState::setWorldTransform(const btTransform& worldTrans)
{
	m_transform = worldTrans;
}

void DynamicAndBoneMergeMotionState::Reset()
{
	XMMATRIX global = m_offset * m_pBoneNode->GetGlobalTransform();
	float m[16];
	GetTransposeMatrix(global, m);
	m_transform.setFromOpenGLMatrix(m);
}

void DynamicAndBoneMergeMotionState::ReflectGlobalTransform()
{
	XMMATRIX world = GetMatrixFrombtTransform(m_transform);
	XMMATRIX result = m_invOffset * world;
	XMMATRIX global = m_pBoneNode->GetGlobalTransform();

	result.r[3] = global.r[3];

	if (m_bOverride == true)
	{
		m_pBoneNode->SetGlobalTransform(result);
		m_pBoneNode->UpdateChildTransform();
	}
}
