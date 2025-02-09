#include "BulletPhysics.h"

using namespace DirectX;

BulletPhysics::BulletPhysics(XMFLOAT3 gravity)
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
}

BulletPhysics::~BulletPhysics()
{
    delete m_pDynamicsWorld;
    delete m_pSolver;
    delete m_pBroadphase;
    delete m_pDispatcher;
    delete m_pCollisionConfig;
}

btDiscreteDynamicsWorld* BulletPhysics::GetDynamicWorld() const
{
    return m_pDynamicsWorld;
}

void BulletPhysics::Update(float deltaTime)
{
    m_pDynamicsWorld->stepSimulation(deltaTime, 10);
}
