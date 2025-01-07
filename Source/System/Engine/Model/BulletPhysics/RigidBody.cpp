#include "RigidBody.h"

using namespace DirectX;

RigidBody::RigidBody(btDiscreteDynamicsWorld* pDynamicWorld)
	: m_pDynamicWorld(pDynamicWorld)
	, m_pShape(nullptr)
	, m_pActiveMotionState(nullptr)
	, m_pKinematicMotionState(nullptr)
	, m_pRigidBody(nullptr)
{
}

bool RigidBody::Init(RigidBodyShape shape, Bone* pBoneNode, const DirectX::XMFLOAT3&& shapeSize, float mass)
{
	m_pShape = nullptr;

	switch (shape)
	{
	case RigidBodyShape::ShapeSphere:
		m_pShape = new btSphereShape(shapeSize.x);
		break;
	case RigidBodyShape::ShapeBox:
		m_pShape = new btBoxShape(btVector3(shapeSize.x, shapeSize.y, shapeSize.z));
		break;
	case RigidBodyShape::ShapeCapsule:
		m_pShape = new btCapsuleShape(shapeSize.x, shapeSize.y);
		break;
	default:
		break;
	}

	if (!m_pShape) {
		return false;
	}

	btVector3 localInertia(0.0f, 0.0f, 0.9f);

	m_pShape->calculateLocalInertia(mass, localInertia);

	btMotionState* motionState = nullptr;

	m_pActiveMotionState = new DynamicMotionState(pBoneNode, pBoneNode->m_boneOffset);
	m_pKinematicMotionState = new KinematicMotionState(pBoneNode, pBoneNode->m_boneOffset);
	motionState = m_pActiveMotionState;

	btRigidBody::btRigidBodyConstructionInfo rigidBodyInfo(mass, motionState, m_pShape, localInertia);
	rigidBodyInfo.m_additionalDamping = true;

	m_pRigidBody = new btRigidBody(rigidBodyInfo);
	m_pRigidBody->setUserPointer(this);
	m_pRigidBody->setSleepingThresholds(0.01f, XMConvertToRadians(0.1f));
	m_pRigidBody->setActivationState(DISABLE_DEACTIVATION);

	m_pBoneNode = pBoneNode;

	m_pDynamicWorld->addRigidBody(m_pRigidBody);

	return true;
}

void RigidBody::SetActive(bool active)
{
	if (active == true) {
		m_pRigidBody->setMotionState(m_pActiveMotionState);
	}
	else {
		m_pRigidBody->setMotionState(m_pKinematicMotionState);
	}
}

void RigidBody::Reset(btDiscreteDynamicsWorld* world)
{
	btOverlappingPairCache* cache = world->getPairCache();
	if (cache != nullptr) {
		btDispatcher* dispatcher = world->getDispatcher();
		cache->cleanProxyFromPairs(m_pRigidBody->getBroadphaseHandle(), dispatcher);
	}

	m_pRigidBody->setAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));
	m_pRigidBody->setLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));
	m_pRigidBody->clearForces();
}

void RigidBody::ResetTransform()
{
	if (m_pActiveMotionState != nullptr) {
		m_pActiveMotionState->Reset();
	}
}

void RigidBody::ReflectGlobalTransform()
{
	if (m_pActiveMotionState != nullptr) {
		m_pActiveMotionState->ReflectGlobalTransform();
	}

	if (m_pKinematicMotionState != nullptr)
	{
		m_pKinematicMotionState->ReflectGlobalTransform();
	}
}
