#include "HairPhysicsBone.h"

using namespace DirectX;

HairPhysicsBone::HairPhysicsBone()
    : m_pPhysics(nullptr)
    , m_pScene(nullptr)
    , m_pBoneManager(nullptr)
    , m_pRootBone(nullptr)
{
}

HairPhysicsBone::~HairPhysicsBone()
{
    for (HairBone& hb : m_hairBones) {
        if (hb.joint) {
            hb.joint->release();
        }
        if (hb.rigidBody) {
            hb.rigidBody->release();
        }
    }
    m_hairBones.clear();
}

void HairPhysicsBone::Initialize(PxPhysics* pPhysics, PxScene* pScene)
{
    m_pPhysics = pPhysics;
    m_pScene = pScene;
}

void HairPhysicsBone::SetBone(BoneManager* pBoneManager, Bone* pRootBone)
{
    m_pBoneManager = pBoneManager;
    m_pRootBone = pRootBone;

    CreateBoneRecursive(pRootBone, nullptr);
}

void HairPhysicsBone::CreateBoneRecursive(Bone* pBone, Bone* pParentBone)
{
    XMMATRIX offset = pBone->GetBoneOffsetWithModelMatrix();
    XMVECTOR pos = offset.r[3];

    XMFLOAT3 position;
    XMStoreFloat3(&position, pos); // XMFLOAT3 に変換
    XMVECTOR rotQuat = XMQuaternionRotationMatrix(offset);
    PxQuat rot = PxQuat(XMVectorGetX(rotQuat), XMVectorGetY(rotQuat), XMVectorGetZ(rotQuat), XMVectorGetW(rotQuat));
    PxTransform rigidTransform = PxTransform(PxVec3(position.x, position.y, position.z));

    //剛体作成
    PxRigidDynamic* rigidBody = m_pPhysics->createRigidDynamic(rigidTransform);
    PxSphereGeometry sphere(0.02f); //髪の物理サイズ（調整可）
    PxShape* shape = m_pPhysics->createShape(sphere, *m_pPhysics->createMaterial(0.0f, 0.9f, 0.05f));
    rigidBody->attachShape(*shape);
    shape->release();
    rigidBody->setMass(0.1f); //質量（調整可）
    if (!pParentBone) {
        rigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    }
    m_pScene->addActor(*rigidBody);

    //登録
    HairBone hb = { pBone, rigidBody, nullptr };
    m_hairBones.push_back(hb);
    //親がいる場合はジョイント作成
    if (pParentBone) {
        PxRigidDynamic* parentRigid = nullptr;
        for (const HairBone& parentHb : m_hairBones) {
            if (parentHb.pBone == pParentBone) {
                parentRigid = parentHb.rigidBody;
                break;
            }
        }

        if (parentRigid) {
            // 親ボーン
            XMMATRIX parentOffset = pParentBone->GetBoneOffsetWithModelMatrix();
            XMVECTOR parentPos = parentOffset.r[3];

            XMVECTOR parentRotQuat = XMQuaternionRotationMatrix(parentOffset);
            PxQuat parentRot = PxQuat(XMVectorGetX(parentRotQuat), XMVectorGetY(parentRotQuat), XMVectorGetZ(parentRotQuat), XMVectorGetW(parentRotQuat));
            PxTransform localFrame0 = PxTransform(PxVec3(0.0f, 0.0f, 0.0f));

            // 相対ベクトル計算
            XMVECTOR relativePos = parentPos - pos;

            XMFLOAT3 childOffsetVec;
            XMStoreFloat3(&childOffsetVec, relativePos);

            PxVec3 localAnchor = PxVec3(childOffsetVec.x, childOffsetVec.y, childOffsetVec.z);
            PxTransform localFrame1 = PxTransform(localAnchor);

            PxD6Joint* joint = PxD6JointCreate(*m_pPhysics, parentRigid, localFrame0, rigidBody, localFrame1);

            PxJointLimitCone swingLimit(PxPi / 36.0f, PxPi / 36.0f, 0.01f);
            joint->setSwingLimit(swingLimit);
            joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLIMITED);
            joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLOCKED);
            joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLOCKED);      //必要なら制限

            hb.joint = joint;
        }
    }

    //子ボーンも処理
    UINT childCount = pBone->GetChildBoneCount();
    for (UINT i = 0; i < childCount; i++) {
        CreateBoneRecursive(pBone->GetChildBone(i), pBone);
    }
}

void HairPhysicsBone::Update()
{
    SyncBoneRecursive(m_pRootBone);

    if (m_hairBones.size() > 0) {
        XMVECTOR pos, rotQuat, scale;
        XMMatrixDecompose(&scale, &rotQuat, &pos, m_hairBones[0].pBone->GetGlobalTransform());

        PxVec3 physxPos = PxVec3(XMVectorGetX(pos), XMVectorGetY(pos), XMVectorGetZ(pos));
        PxQuat physxRot = PxQuat(XMVectorGetX(rotQuat), XMVectorGetY(rotQuat), XMVectorGetZ(rotQuat), XMVectorGetW(rotQuat));
        PxTransform newTransform(physxPos, physxRot);

        m_hairBones[0].rigidBody->setKinematicTarget(newTransform);
    }

    //m_pBoneManager->UpdateBoneMatrix(m_pRootBone->GetBoneIndex());
}

void HairPhysicsBone::SyncBoneTransform(Bone* pBone)
{
    //対応する剛体を探す
    for (auto& hb : m_hairBones) {
        if (hb.pBone == pBone) {
            //剛体のグローバルポーズ取得
            PxTransform transform = hb.rigidBody->getGlobalPose();
            //位置を反映
            //pBone->m_position = XMFLOAT3(transform.p.x, transform.p.y, transform.p.z);

            //回転を反映 (PhysXはクォータニオン)
            pBone->m_rotation = XMFLOAT4(transform.q.x, transform.q.y, transform.q.z, transform.q.w);

            break; //該当ボーン見つかったら終了
        }
    }
}

void HairPhysicsBone::SyncBoneRecursive(Bone* pBone)
{
    //まずこのボーンを同期
    SyncBoneTransform(pBone);

    //子ボーンを再帰的に同期
    UINT childCount = pBone->GetChildBoneCount();
    for (UINT i = 0; i < childCount; ++i) {
        Bone* childBone = pBone->GetChildBone(i);
        SyncBoneRecursive(childBone);
    }
}
