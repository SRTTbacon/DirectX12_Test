#pragma once
#include <atomic>
#include <thread>
#include <Windows.h>

#include <btBulletDynamicsCommon.h>
#include <DirectXMath.h>

class BulletPhysics
{
public:
    BulletPhysics(DirectX::XMFLOAT3 gravity);
    ~BulletPhysics();

    void SetActive(bool bActive);

public:
    btDiscreteDynamicsWorld* GetDynamicWorld() const;

private:
    btDefaultCollisionConfiguration* m_pCollisionConfig;
    btCollisionDispatcher* m_pDispatcher;
    btBroadphaseInterface* m_pBroadphase;
    btSequentialImpulseConstraintSolver* m_pSolver;
    static btDiscreteDynamicsWorld* m_pDynamicsWorld;

    std::thread m_physicsUpdateThread;

    static std::atomic<bool> m_bStopFlag;
    static std::atomic<bool> m_bEndFlag;

    bool m_bThreadFlag;

    static void UpdateByThread();
};