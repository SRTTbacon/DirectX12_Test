#pragma once

#include "BulletPhysics.h"

#include "..\\Bone\\Bone.h"

class DynamicBone {
public:
    DynamicBone(btDiscreteDynamicsWorld* pWorld, BoneManager* pBoneManager);
    ~DynamicBone();

    void Initialize(Bone* pRootBone, float stiffness, float damping);
    void Update();

private:
    btDiscreteDynamicsWorld* m_pWorld;
    BoneManager* m_pBoneManager;
    std::vector<btRigidBody*> m_rigidBodies;
    std::vector<Bone*> m_bones;
    std::vector<btTypedConstraint*> m_constraints;
    Bone* m_pRootBone;

    void InitializeChildBone(Bone* pParentBone, float stiffness, float damping);
    btRigidBody* CreateRigidBody(Bone* pBone);
    void CreateConstraint(btRigidBody* pParentBody, btRigidBody* pChildBody, float stiffness, float damping);
};