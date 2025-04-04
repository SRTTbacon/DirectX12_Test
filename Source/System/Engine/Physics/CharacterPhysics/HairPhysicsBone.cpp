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
    XMStoreFloat3(&position, pos); // XMFLOAT3 �ɕϊ�
    XMVECTOR rotQuat = XMQuaternionRotationMatrix(offset);
    PxQuat rot = PxQuat(XMVectorGetX(rotQuat), XMVectorGetY(rotQuat), XMVectorGetZ(rotQuat), XMVectorGetW(rotQuat));
    PxTransform rigidTransform = PxTransform(PxVec3(position.x, position.y, position.z));

    //���̍쐬
    PxRigidDynamic* rigidBody = m_pPhysics->createRigidDynamic(rigidTransform);
    PxSphereGeometry sphere(0.02f); //���̕����T�C�Y�i�����j
    PxShape* shape = m_pPhysics->createShape(sphere, *m_pPhysics->createMaterial(0.0f, 0.9f, 0.05f));
    rigidBody->attachShape(*shape);
    shape->release();
    rigidBody->setMass(0.1f); //���ʁi�����j
    if (!pParentBone) {
        rigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    }
    m_pScene->addActor(*rigidBody);

    //�o�^
    HairBone hb = { pBone, rigidBody, nullptr };
    m_hairBones.push_back(hb);
    //�e������ꍇ�̓W���C���g�쐬
    if (pParentBone) {
        PxRigidDynamic* parentRigid = nullptr;
        for (const HairBone& parentHb : m_hairBones) {
            if (parentHb.pBone == pParentBone) {
                parentRigid = parentHb.rigidBody;
                break;
            }
        }

        if (parentRigid) {
            // �e�{�[��
            XMMATRIX parentOffset = pParentBone->GetBoneOffsetWithModelMatrix();
            XMVECTOR parentPos = parentOffset.r[3];

            XMVECTOR parentRotQuat = XMQuaternionRotationMatrix(parentOffset);
            PxQuat parentRot = PxQuat(XMVectorGetX(parentRotQuat), XMVectorGetY(parentRotQuat), XMVectorGetZ(parentRotQuat), XMVectorGetW(parentRotQuat));
            PxTransform localFrame0 = PxTransform(PxVec3(0.0f, 0.0f, 0.0f));

            // ���΃x�N�g���v�Z
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
            joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLOCKED);      //�K�v�Ȃ琧��

            hb.joint = joint;
        }
    }

    //�q�{�[��������
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
    //�Ή����鍄�̂�T��
    for (auto& hb : m_hairBones) {
        if (hb.pBone == pBone) {
            //���̂̃O���[�o���|�[�Y�擾
            PxTransform transform = hb.rigidBody->getGlobalPose();
            //�ʒu�𔽉f
            //pBone->m_position = XMFLOAT3(transform.p.x, transform.p.y, transform.p.z);

            //��]�𔽉f (PhysX�̓N�H�[�^�j�I��)
            pBone->m_rotation = XMFLOAT4(transform.q.x, transform.q.y, transform.q.z, transform.q.w);

            break; //�Y���{�[������������I��
        }
    }
}

void HairPhysicsBone::SyncBoneRecursive(Bone* pBone)
{
    //�܂����̃{�[���𓯊�
    SyncBoneTransform(pBone);

    //�q�{�[�����ċA�I�ɓ���
    UINT childCount = pBone->GetChildBoneCount();
    for (UINT i = 0; i < childCount; ++i) {
        Bone* childBone = pBone->GetChildBone(i);
        SyncBoneRecursive(childBone);
    }
}
