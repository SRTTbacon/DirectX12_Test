#pragma once
#include <PxPhysicsAPI.h>

using namespace physx;

class PhysXManager
{
public:
	PhysXManager();

	void SetFPS(float frameRate);

	void Update();

private:
	PxDefaultAllocator m_allocator;
	PxDefaultErrorCallback m_errorCallback;
	PxFoundation* m_pFoundation;
	PxPhysics* m_pPhysics;
	PxDefaultCpuDispatcher* m_pDispatcher;
	PxScene* m_pScene;
	PxPvd* m_pPvd;

	PxPvdSceneClient* m_pPvdClient;

	float m_frameRate;
};