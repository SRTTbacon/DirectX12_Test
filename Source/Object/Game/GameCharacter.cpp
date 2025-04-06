#include "GameCharacter.h"

#include "..\\..\\System\\Engine\\Model\\Convert\\ConvertFromFBX.h"

const std::string GameCharacter::CHARACTER_FILE = "Resource/Model/FBX_Moe.hcs";

GameCharacter::GameCharacter()
	: m_pPoseChar(nullptr)
	, m_pDanceChar1(nullptr)
	, m_pDanceChar2(nullptr)
	, m_pBGMHandle(nullptr)
	, m_blinkMode(BlinkNone)
	, m_animSpeed(1.0f)
	, m_nowBlinkTime(0.0f)
	, m_nextBlinkTime(0.0f)
	, m_bAnim(true)
{
}

void GameCharacter::Initialize()
{
	//�_�����L�����N�^�[
	m_pPoseChar = g_Engine->AddCharacter(CHARACTER_FILE);
	m_pPoseChar->AddAnimation(g_Engine->GetAnimation("Resource\\Pose.hcs"));
	m_pPoseChar->SetRotation(-90.0f, 180.0f, 0.0f);
	m_pPoseChar->m_position.y = -1.08f;
	m_pPoseChar->SetShapeWeight("���E_��", 1.0f);

	m_pPoseChar->GetBone("Back Hair.001")->m_boneType = BONETYPE_LEFT_HAIR;
	m_pPoseChar->GetBone("Back Hair.002")->m_boneType = BONETYPE_LEFT_HAIR;
	m_pPoseChar->GetBone("Back Hair.003")->m_boneType = BONETYPE_LEFT_HAIR;

	for (UINT i = 0; i < m_pPoseChar->GetMeshCount(); i++) {
		Mesh* pMesh = m_pPoseChar->GetMesh(i);
		pMesh->pMaterial->m_ambientColor = XMFLOAT4(0.2f, 0.25f, 0.9f, 1.0f);

		if (pMesh->meshName == "Hair" || pMesh->meshName == "Ear") {
			pMesh->pMaterial->m_diffuseColor = XMFLOAT4(1.0f, 1.0f, 1.5f, 1.0f);
		}
	}

	//�����L�����N�^�[��z�u���A�A�j���[�V������ύX
	m_pDanceChar1 = g_Engine->AddCharacter(CHARACTER_FILE);
	m_pDanceChar1->AddAnimation(g_Engine->GetAnimation("Resource\\Roki1.hcs"));
	m_pDanceChar1->m_animationSpeed = m_animSpeed;
	m_pDanceChar1->SetRotation(-90.0f, 180.0f, 0.0f);
	m_pDanceChar1->m_position.x = 5.0f;
	m_pDanceChar1->m_position.y = -1.08f;
	m_pDanceChar1->SetShapeWeight("���E_��", 1.0f);

	//�����L�����N�^�[��z�u���A�A�j���[�V������ύX
	m_pDanceChar2 = g_Engine->AddCharacter(CHARACTER_FILE);
	m_pDanceChar2->AddAnimation(g_Engine->GetAnimation("Resource\\Roki2.hcs"));
	m_pDanceChar2->m_animationSpeed = m_animSpeed;
	m_pDanceChar2->SetRotation(-90.0f, 180.0f, 0.0f);
	m_pDanceChar2->m_position.x = 5.0f;
	m_pDanceChar2->m_position.y = -1.08f;
	m_pDanceChar2->SetShapeWeight("���E_��", 1.0f);

	/*m_pBGMHandle = g_Engine->GetBassSoundSystem()->LoadSound("Resource\\BGM\\Roki.mp3", false);
	m_pBGMHandle->m_volume = 0.1f;
	m_pBGMHandle->m_speed = m_animSpeed;
	m_pBGMHandle->m_bLooping = true;
	m_pBGMHandle->UpdateProperty();*/

	//�_�����L�����N�^�[�̎��̏u���̎��Ԃ��w��
	SetNextBlinkTime();

	//.fbx�̃q���[�}�m�C�h���f����.hcs�ɕϊ�
	//ConvertFromFBX convert;
	//convert.ConvertFromCharacter(m_pPoseChar, CHARACTER_FILE, "Resource/Model/FBX_Moe.hcs");
}

void GameCharacter::Update()
{
	//�L�[������X�V
	UpdateKeys();
	//�_�����L�����N�^�[�̂܂΂���
	UpdateBlink();

	//�e�L�����N�^�[�̍X�V
	m_pPoseChar->Update();
	m_pDanceChar1->Update();
	m_pDanceChar2->Update();
}

void GameCharacter::UpdateKeys()
{
	//���N���b�N�ŃA�j���[�V�������ꎞ��~ / �ĊJ
	if (g_Engine->GetMouseStateSync(0x00)) {
		m_bAnim = !m_bAnim;
		if (m_bAnim) {
			m_pDanceChar1->m_animationSpeed = m_animSpeed;
			m_pDanceChar2->m_animationSpeed = m_animSpeed;
			//m_pBGMHandle->PlaySound(false);
		}
		else {
			m_pDanceChar1->m_animationSpeed = 0.0f;
			m_pDanceChar2->m_animationSpeed = 0.0f;
			//m_pBGMHandle->PauseSound();
		}
	}

	//�E�N���b�N�ŃL�����N�^�[����]
	if (g_Engine->GetMouseState(0x01)) {
		m_pDanceChar1->AddRotationY(100.0f * g_Engine->GetFrameTime());
		m_pDanceChar2->AddRotationY(100.0f * g_Engine->GetFrameTime());
	}
	if (g_Engine->GetMouseStateSync(0x02)) {
		//m_pBGMHandle->SetPosition(m_pDanceChar1->m_nowAnimationTime);
	}

	//�A�j���[�V������5�b�߂��A5�b�i��
	if (g_Engine->GetKeyStateSync(DIK_J)) {
		m_pDanceChar1->m_nowAnimationTime -= 5.0f;
		m_pDanceChar2->m_nowAnimationTime -= 5.0f;
		if (m_pDanceChar1->m_nowAnimationTime < 0.0f) {
			m_pDanceChar1->m_nowAnimationTime = 0.0f;
			m_pDanceChar2->m_nowAnimationTime = 0.0f;
		}

		//m_pBGMHandle->SetPosition(m_pDanceChar1->m_nowAnimationTime);
	}
	else if (g_Engine->GetKeyStateSync(DIK_K)) {
		m_pDanceChar1->m_nowAnimationTime += 5.0f;
		m_pDanceChar2->m_nowAnimationTime += 5.0f;

		//m_pBGMHandle->SetPosition(m_pDanceChar1->m_nowAnimationTime);
	}

	//G �܂��� H�L�[�ŃA�j���[�V�������x��ύX
	if (g_Engine->GetKeyState(DIK_G)) {
		m_animSpeed -= 0.35f * g_Engine->GetFrameTime();

		//0�ɂ͂��Ȃ�
		if (m_animSpeed < 0.1f)
			m_animSpeed = 0.1f;

		m_pDanceChar1->m_animationSpeed = m_animSpeed;
		m_pDanceChar2->m_animationSpeed = m_animSpeed;
		//m_pBGMHandle->m_speed = m_animSpeed;
		//m_pBGMHandle->UpdateProperty();
	}
	if (g_Engine->GetKeyState(DIK_H)) {
		m_animSpeed += 0.35f * g_Engine->GetFrameTime();

		//2�{�ȏ�ɂ͂��Ȃ�
		if (m_animSpeed > 2.0f)
			m_animSpeed = 2.0f;

		m_pDanceChar1->m_animationSpeed = m_animSpeed;
		m_pDanceChar2->m_animationSpeed = m_animSpeed;
		//m_pBGMHandle->m_speed = m_animSpeed;
		//m_pBGMHandle->UpdateProperty();
	}
}

void GameCharacter::UpdateBlink()
{
	m_nowBlinkTime += g_Engine->GetFrameTime();
	if (m_nowBlinkTime >= m_nextBlinkTime) {
		SetNextBlinkTime();
		m_nowBlinkTime = 0.0f;
		m_blinkMode = BlinkStart;
	}

	if (m_blinkMode == BlinkStart) {
		float shapeValue = m_pPoseChar->GetShapeWeight("�܂΂���");
		shapeValue += g_Engine->GetFrameTime() * 7.5f;
		if (shapeValue >= 1.0f) {
			shapeValue = 1.0f;
			m_blinkMode = BlinkEnd;
		}
		m_pPoseChar->SetShapeWeight("�܂΂���", shapeValue);
	}
	else if (m_blinkMode == BlinkEnd) {
		float shapeValue = m_pPoseChar->GetShapeWeight("�܂΂���");
		shapeValue -= g_Engine->GetFrameTime() * 7.5f;
		if (shapeValue <= 0.0f) {
			shapeValue = 0.0f;
			m_blinkMode = BlinkNone;
		}
		m_pPoseChar->SetShapeWeight("�܂΂���", shapeValue);
	}
}

void GameCharacter::SetNextBlinkTime()
{
	//2�`5�b�̊ԂŃ����_���ɏu��������
	float randomTime = rand() % 3001 / 1000.0f;
	m_nextBlinkTime = 2.0f + randomTime;
}
