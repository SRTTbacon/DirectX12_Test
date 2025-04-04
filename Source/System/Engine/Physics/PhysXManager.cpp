#include "PhysXManager.h"

//---------検証中のため使用不可---------

using namespace physx;

PhysXManager::PhysXManager()
{
    m_pFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_allocator, m_errorCallback);

    // PVDの設定
    m_pPvd = PxCreatePvd(*m_pFoundation);
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
    m_pPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

    m_pPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_pFoundation, PxTolerancesScale(), true, m_pPvd);
    PxInitExtensions(*m_pPhysics, m_pPvd);

    // Sceneの作成
    PxSceneDesc sceneDesc(m_pPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.8f, 0.0f);
    m_pDispatcher = PxDefaultCpuDispatcherCreate(0);
    sceneDesc.cpuDispatcher = m_pDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    m_pScene = m_pPhysics->createScene(sceneDesc);

    // PVDの設定
    m_pPvdClient = m_pScene->getScenePvdClient();
    if (m_pPvdClient)
    {
        m_pPvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        m_pPvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        m_pPvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }
}

void PhysXManager::Update(float deltaTime)
{
    m_pScene->simulate(deltaTime);
    m_pScene->fetchResults(true);
}
