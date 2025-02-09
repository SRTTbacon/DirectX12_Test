#include "DynamicBone.h"

using namespace DirectX;

DynamicBone::DynamicBone(btDiscreteDynamicsWorld* pWorld, BoneManager* pBoneManager)
    : m_pWorld(pWorld)
    , m_pBoneManager(pBoneManager)
    , m_pRootBone(nullptr)
{
}

DynamicBone::~DynamicBone()
{
    // 剛体と制約の解放
    for (auto body : m_rigidBodies) {
        m_pWorld->removeRigidBody(body);
        delete body->getMotionState();
        delete body;
    }
    for (auto constraint : m_constraints) {
        m_pWorld->removeConstraint(constraint);
        delete constraint;
    }
    m_rigidBodies.clear();
    m_constraints.clear();
}

void DynamicBone::Initialize(Bone* pRootBone, float stiffness, float damping)
{
    m_pRootBone = pRootBone;

    // 剛体を作成
    InitializeChildBone(pRootBone, stiffness, damping);
}

void DynamicBone::InitializeChildBone(Bone* pParentBone, float stiffness, float damping)
{
    btRigidBody* pRigidBody = CreateRigidBody(pParentBone);
    pParentBone->m_pRigidBody = pRigidBody;
    m_rigidBodies.push_back(pRigidBody);
    m_bones.push_back(pParentBone);

    for (UINT i = 0; i < pParentBone->GetChildBoneCount(); i++) {
        Bone* pChildBone = pParentBone->GetChildBone(i);
        InitializeChildBone(pChildBone, stiffness, damping);
    }

    if (pParentBone != m_pRootBone && pParentBone->GetParentBone() != m_pRootBone) {
        CreateConstraint(pParentBone->GetParentBone()->m_pRigidBody, pRigidBody, stiffness, damping);
    }
}

btRigidBody* DynamicBone::CreateRigidBody(Bone* pBone)
{
    XMMATRIX mat = pBone->GetGlobalTransform();
    XMVECTOR scale, quaternion, position;
    XMMatrixDecompose(&scale, &quaternion, &position, mat);
    btQuaternion btQua = btQuaternion(XMVectorGetX(quaternion), XMVectorGetY(quaternion), XMVectorGetZ(quaternion), XMVectorGetW(quaternion));
    btVector3 btPos = btVector3(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position));
    btTransform transform = btTransform(btQua, btPos);
    //printf("%s = %f, %f, %f\n", pBone->GetBoneName().c_str(), btPos.x(), btPos.y(), btPos.z());

    //剛体の設定
    btDefaultMotionState* pMotionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, pMotionState, nullptr, btVector3(0, 0, 0));
    btRigidBody* pRigidBody = new btRigidBody(rbInfo);
    pRigidBody->setActivationState(DISABLE_DEACTIVATION); //常にアクティブ状態
    m_pWorld->addRigidBody(pRigidBody);

    return pRigidBody;
}

void DynamicBone::CreateConstraint(btRigidBody* pParentBody, btRigidBody* pChildBody, float stiffness, float damping)
{
    btTransform frameInA, frameInB;
    frameInA = btTransform::getIdentity();
    frameInA.getBasis().setEulerZYX(0, 0, SIMD_2_PI);
    frameInA.setOrigin(btVector3(btScalar(0.), btScalar(-5.), btScalar(0.)));
    frameInB = btTransform::getIdentity();
    frameInB.getBasis().setEulerZYX(0, 0, SIMD_2_PI);
    frameInB.setOrigin(btVector3(btScalar(0.), btScalar(5.), btScalar(0.)));
    btConeTwistConstraint* pConstraint = new btConeTwistConstraint(*pParentBody, *pChildBody, frameInA, frameInB);

    // 制約のパラメータ設定（柔軟性）
    pConstraint->setLimit(
        SIMD_PI / 4, // 回転角度の制限 (45度)
        SIMD_PI / 4,
        stiffness,    // 剛性
        damping,      // 減衰率
        0.1f          // ソフトネス
    );

    //制約をワールドに追加
    m_pWorld->addConstraint(pConstraint);
    m_constraints.push_back(pConstraint);
}

void DynamicBone::Update()
{
    for (size_t i = 0; i < m_rigidBodies.size(); ++i) {
        btTransform transform;
        m_rigidBodies[i]->getMotionState()->getWorldTransform(transform);

        //btQuaternion btQua = transform.getRotation();
        // BulletのTransformをDirectXの行列に変換
        btScalar matrix[16];
        transform.getOpenGLMatrix(matrix);
        XMMATRIX m = DirectX::XMMATRIX(matrix);
        //m = XMMatrixTranspose(m);
        XMVECTOR scale, rot, pos;
        XMMatrixDecompose(&scale, &rot, &pos, m);
        //printf("x = %f, y = %f, z = %f, w = %f\n", btQua.x(), btQua.y(), btQua.z(), btQua.w());
        //XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(XMVectorSet(btQua.x(), btQua.y(), btQua.z(), btQua.w()));
        //XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rot);
        //XMMATRIX boneMatrix = m_pBoneManager->m_boneInfos[m_bones[i]->GetBoneIndex()];
        //boneMatrix *= rotationMatrix;
        //m_pBoneManager->m_boneInfos[m_bones[i]->GetBoneIndex()] = boneMatrix;
        XMStoreFloat4(&m_bones[i]->m_rotation, rot);
        float temp = m_bones[i]->m_rotation.x;
        m_bones[i]->m_rotation.x = m_bones[i]->m_rotation.y;
        m_bones[i]->m_rotation.y = temp;
        if (m_bones[i]->GetBoneName() == "Hair Root") {
            printf("x = %f, y = %f, z = %f, w = %f\n", m_bones[i]->m_rotation.x, m_bones[i]->m_rotation.y, m_bones[i]->m_rotation.z, m_bones[i]->m_rotation.w);
        }
        m_pBoneManager->UpdateBoneMatrix(m_bones[i]->GetBoneIndex());
        //for (UINT j = 0; j < m_bones[i]->GetChildBoneCount(); j++) {
            //m_pBoneManager->UpdateBoneMatrix(m_bones[i]->GetChildBone(j)->GetBoneIndex());
        //}
        //m_bones[i]->m_position.x = XMVectorGetX(vectorTranslation) - m_bones[i]->m_initPos.x;
        //m_bones[i]->m_position.y = XMVectorGetY(vectorTranslation) - m_bones[i]->m_initPos.y;
        //m_bones[i]->m_position.z = XMVectorGetZ(vectorTranslation) - m_bones[i]->m_initPos.z;
    }
}
