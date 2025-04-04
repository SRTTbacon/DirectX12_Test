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

    void SetBone(BoneManager* pBoneManager, Bone* pRootBone); //髪の親ボーンを渡す
    void Update(); //物理シミュレーション結果からボーンの回転反映

private:
    void CreateBoneRecursive(Bone* pBone, Bone* pParentBone); //再帰的に物理ボーン生成
    void SyncBoneTransform(Bone* pBone); //物理結果をボーンに適応
    void SyncBoneRecursive(Bone* pBone); //再帰的に同期処理

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