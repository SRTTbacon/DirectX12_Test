#pragma once
#include <PxPhysicsAPI.h>

//---------åüèÿíÜÇÃÇΩÇﬂégópïsâ¬---------

using namespace physx;

class PhysXManager
{
public:
	PhysXManager();

	void Update(float deltaTime);

public:
	inline PxPhysics* GetPhysics() { return m_pPhysics; }
	inline PxScene* GetScene() { return m_pScene; }

private:
	PxDefaultAllocator m_allocator;
	PxDefaultErrorCallback m_errorCallback;
	PxFoundation* m_pFoundation;
	PxPhysics* m_pPhysics;
	PxDefaultCpuDispatcher* m_pDispatcher;
	PxScene* m_pScene;
	PxPvd* m_pPvd;

	PxPvdSceneClient* m_pPvdClient;
};