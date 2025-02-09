#pragma once
#include <btBulletDynamicsCommon.h>
#include <DirectXMath.h>

class BulletPhysics
{
public:
    BulletPhysics(DirectX::XMFLOAT3 gravity);
    ~BulletPhysics();

    void Update(float deltaTime);

public:
    btDiscreteDynamicsWorld* GetDynamicWorld() const;

private:
    btDefaultCollisionConfiguration* m_pCollisionConfig;
    btCollisionDispatcher* m_pDispatcher;
    btBroadphaseInterface* m_pBroadphase;
    btSequentialImpulseConstraintSolver* m_pSolver;
    btDiscreteDynamicsWorld* m_pDynamicsWorld;
};