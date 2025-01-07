#include "BulletPhysics.h"

using namespace DirectX;

btDiscreteDynamicsWorld* BulletPhysics::m_pDynamicsWorld = nullptr;
std::atomic<bool> BulletPhysics::m_bStopFlag(false);
std::atomic<bool> BulletPhysics::m_bEndFlag(false);

BulletPhysics::BulletPhysics(XMFLOAT3 gravity)
    : m_bThreadFlag(false)
{
    //初期化: ダイナミクスワールドを作成
    m_pCollisionConfig = new btDefaultCollisionConfiguration();
    m_pDispatcher = new btCollisionDispatcher(m_pCollisionConfig);
    m_pBroadphase = new btDbvtBroadphase();
    m_pSolver = new btSequentialImpulseConstraintSolver();
    m_pDynamicsWorld = new btDiscreteDynamicsWorld(m_pDispatcher, m_pBroadphase, m_pSolver, m_pCollisionConfig);
    m_pDynamicsWorld->setGravity(btVector3(gravity.x, gravity.y, gravity.z)); //重力を設定

    btContactSolverInfo& info = m_pDynamicsWorld->getSolverInfo();
    info.m_numIterations = 4;
    info.m_solverMode = SOLVER_SIMD;

    SetActive(true);
}

BulletPhysics::~BulletPhysics()
{
    if (m_bThreadFlag == true)
    {
        m_bStopFlag.store(true);

        while (m_bEndFlag.load() == false);
    }

    delete m_pDynamicsWorld;
    delete m_pSolver;
    delete m_pBroadphase;
    delete m_pDispatcher;
    delete m_pCollisionConfig;
}

void BulletPhysics::SetActive(bool bActive)
{
    m_bStopFlag.store(!bActive);
    m_bThreadFlag = bActive;

    if (bActive)
    {
        m_bEndFlag.store(false);
        m_physicsUpdateThread = std::thread(UpdateByThread);
        m_physicsUpdateThread.detach();
    }
    else
    {
        while (m_bEndFlag.load() == false);
    }
}

btDiscreteDynamicsWorld* BulletPhysics::GetDynamicWorld() const
{
    return m_pDynamicsWorld;
}

void BulletPhysics::UpdateByThread()
{
    unsigned long prevTime = timeGetTime();

    while (true)
    {
        if (m_bStopFlag.load() == true)
        {
            break;
        }

        unsigned long currentTime = timeGetTime();
        unsigned long deltaTime = currentTime - prevTime;
        prevTime = currentTime;

        m_pDynamicsWorld->stepSimulation(deltaTime * 0.001f);
    }

    m_bEndFlag.store(true);
}
