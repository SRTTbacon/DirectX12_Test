#include "Animation.h"

Animation::Animation()
	: m_pTempFrame(nullptr)
{
}

Animation::~Animation()
{
	if (m_pTempFrame) {
		delete m_pTempFrame;
		m_pTempFrame = nullptr;
	}
}

//�t�@�C������A�j���[�V���������[�h
void Animation::Load(std::string animFilePath)
{
	//�o�C�i���Ƃ��ĊJ��
	BinaryReader br(animFilePath);
	USHORT boneCount = br.ReadUInt16();		//�A�j���[�V��������{�[����
	printf("boneCount = %d\n", boneCount);
	for (USHORT i = 0; i < boneCount; i++) {

		std::string boneName = br.ReadBytes(br.ReadByte());	//�{�[����
		boneMapping.push_back(boneName);
	}
	USHORT animCount = br.ReadUInt16();		//�A�j���[�V�����̃t���[����
	printf("animCount = %d\n", animCount);

	for (int i = 0; i < animCount; i++) {
		float time = br.ReadFloat();			//�t���[������
		AnimationFrame frame{ time };
		for (int j = 0; j < boneCount; j++) {
			//��]
			float rotX = br.ReadFloat();
			float rotY = br.ReadFloat();
			float rotZ = br.ReadFloat();
			float rotW = br.ReadFloat();
			//���Έʒu
			float posX = br.ReadFloat();
			float posY = br.ReadFloat();
			float posZ = br.ReadFloat();
			BoneAnimation bone{ DirectX::XMFLOAT3(posX, posY, posZ), DirectX::XMFLOAT4(rotX, rotY, rotZ, rotW) };

			//�z��ɒǉ�
			frame.animations.push_back(bone);
		}

		m_frames.push_back(frame);
	}
}

//�w�肵���A�j���[�V�������Ԃ̃t���[�����擾
AnimationFrame* Animation::GetFrame(float nowAnimTime)
{
	//��ԗp�̃t���[��������΍폜
	if (m_pTempFrame) {
		delete m_pTempFrame;
		m_pTempFrame = nullptr;
	}

	AnimationFrame* currentFrame = nullptr;
	AnimationFrame* nextFrame = nullptr;

	//nowAnimTime�̒��O�̃t���[�����擾
	for (std::vector<AnimationFrame>::iterator it = m_frames.begin(); it != m_frames.end(); it++) {
		if (it->time <= nowAnimTime) {
			currentFrame = &(*it);

			//���̃t���[��������Ύ擾
			std::vector<AnimationFrame>::iterator nextIt = std::next(it);
			if (nextIt != m_frames.end()) {
				nextFrame = &(*nextIt);
			}
			else {
				nextFrame = nullptr;
				break;
			}
		}
		else {
			break;
		}
	}

	//�t���[�������݂��Ȃ���ΏI��
	if (currentFrame == nullptr) {
		return nullptr;
	}
	//���̃t���[�������݂��Ȃ���΍Ō�̃t���[�������̂܂ܕԂ�
	if (!nextFrame) {
		return currentFrame;
	}

	//Lerp�p (0.0f�`1.0f)
	float t = (nowAnimTime - currentFrame->time) / (nextFrame->time - currentFrame->time);

	//���݂̃t���[���Ǝ��̃t���[���̊Ԃ���
	m_pTempFrame = new AnimationFrame();
	m_pTempFrame->time = nowAnimTime;
	for (UINT i = 0; i < currentFrame->animations.size(); i++) {
		BoneAnimation lerpBoneAnim{};
		lerpBoneAnim.position = Lerp(currentFrame->animations[i].position, nextFrame->animations[i].position, t);
		lerpBoneAnim.rotation = Lerp(currentFrame->animations[i].rotation, nextFrame->animations[i].rotation, t);
		m_pTempFrame->animations.push_back(lerpBoneAnim);
	}

	return m_pTempFrame;
}

//�t���[�����Ō�̃t���[�����ǂ���
bool Animation::IsLastFrame(AnimationFrame* pAnimFrame)
{
	if (!pAnimFrame)
		return false;
	return pAnimFrame == &m_frames[m_frames.size() - 1];
}
