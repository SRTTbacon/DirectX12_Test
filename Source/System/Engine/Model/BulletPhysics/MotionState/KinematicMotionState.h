#pragma once

#include "MotionState.h"
#include "..\\..\\Bone\\Bone.h"

class KinematicMotionState : public MotionState
{
public:
	KinematicMotionState(Bone* pBoneNode, const DirectX::XMMATRIX& offset);

	void getWorldTransform(btTransform& worldTrans) const override;

	void setWorldTransform(const btTransform& worldTrans) override;

	void Reset() override;

	void ReflectGlobalTransform() override;

private:
	Bone* m_pBoneNode;
	DirectX::XMMATRIX m_offset;
};
