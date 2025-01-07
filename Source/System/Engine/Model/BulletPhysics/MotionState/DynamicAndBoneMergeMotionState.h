#pragma once

#include "MotionState.h"
#include "..\\..\\Bone\\Bone.h"

class DynamicAndBoneMergeMotionState : public MotionState
{
public:
	DynamicAndBoneMergeMotionState(Bone* pBoneNode, const DirectX::XMMATRIX& offset, bool override = true);

	void getWorldTransform(btTransform& worldTrans) const override;

	void setWorldTransform(const btTransform& worldTrans) override;

	void Reset() override;

	void ReflectGlobalTransform() override;

private:
	Bone* m_pBoneNode;
	DirectX::XMMATRIX m_offset;
	DirectX::XMMATRIX m_invOffset;
	btTransform m_transform;
	bool m_bOverride;
};
