#pragma once

#include <PxPhysicsAPI.h>
#include <DirectXMath.h>
#include <vector>

#include "..\\..\\Model\\Bone\\Bone.h"

using namespace physx;

class HairPhysicsBone
{
public:
    HairPhysicsBone();
    ~HairPhysicsBone();

    void Initialize(PxPhysics* pPhysics, PxScene* pScene);

    void SetBone(BoneManager* pBoneManager, Bone* pRootBone); //���̐e�{�[����n��
    void Update(); //�����V�~�����[�V�������ʂ���{�[���̉�]���f

private:
    void CreateBoneRecursive(Bone* pBone, Bone* pParentBone); //�ċA�I�ɕ����{�[������
    void SyncBoneTransform(Bone* pBone); //�������ʂ��{�[���ɓK��
    void SyncBoneRecursive(Bone* pBone); //�ċA�I�ɓ�������

private:
    struct HairBone
    {
        Bone* pBone;
        PxRigidDynamic* rigidBody;
        PxD6Joint* joint;
    };

    PxPhysics* m_pPhysics;
    PxScene* m_pScene;

    BoneManager* m_pBoneManager;
    Bone* m_pRootBone;

    std::vector<HairBone> m_hairBones;
};