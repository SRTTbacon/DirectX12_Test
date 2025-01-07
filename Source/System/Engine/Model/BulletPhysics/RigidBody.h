#pragma once

#include "MotionState\\DynamicMotionState.h"
#include "MotionState\\KinematicMotionState.h"
#include "..\\Bone\\Bone.h"

enum RigidBodyShape
{
	ShapeSphere,
	ShapeBox,
	ShapeCapsule
};

class RigidBody
{
public:
	RigidBody(btDiscreteDynamicsWorld* pDynamicWorld);
	~RigidBody();

	bool Init(RigidBodyShape shape, Bone* pBoneNode, const DirectX::XMFLOAT3&& shapeSize, float mass);

	btRigidBody* GetRigidBody() const;

	void SetActive(bool active);
	void Reset(btDiscreteDynamicsWorld* world);
	void ResetTransform();
	void ReflectGlobalTransform();
	void CalcLocalTransform();

	DirectX::XMMATRIX& GetTransform();

private:
	btDiscreteDynamicsWorld* m_pDynamicWorld;
	btCollisionShape* m_pShape;
	MotionState* m_pActiveMotionState;
	MotionState* m_pKinematicMotionState;
	btRigidBody* m_pRigidBody;

	Bone* m_pBoneNode;
	DirectX::XMFLOAT4X4 m_offsetMatrix;
};
