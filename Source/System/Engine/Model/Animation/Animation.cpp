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
	printf("boneCount = %u\n", boneCount);
	for (USHORT i = 0; i < boneCount; i++) {

		std::string boneName = br.ReadBytes(br.ReadByte());	//�{�[����
		m_boneMapping.push_back(boneName);
	}
	USHORT animCount = br.ReadUInt16();		//�A�j���[�V�����̃t���[����
	printf("animCount = %u\n", animCount);

	for (int i = 0; i < animCount; i++) {
		float time = br.ReadFloat();			//�t���[������
		AnimationFrame frame(time);
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
			frame.boneAnimations.push_back(bone);
		}

		m_frames.push_back(frame);
	}

	USHORT shapeCount = br.ReadUInt16();	//�V�F�C�v�L�[�̐�
	printf("shapeCount = %u\n", shapeCount);

	for (int i = 0; i < shapeCount; i++) {
		std::string shapeName = UTF8ToShiftJIS(br.ReadBytes(br.ReadByte()));
		m_shapeNames.push_back(shapeName);		//�V�F�C�v�L�[�̖��O��ۑ�
		m_shapeAnimations[shapeName] = std::vector<ShapeAnimation>();

		USHORT frameCount = br.ReadUInt16();	//���̃V�F�C�v�L�[�̃A�j���[�V������

		for (int j = 0; j < frameCount; j++) {
			float time = br.ReadFloat();		//�L�[�̎���
			float value = br.ReadFloat();		//�V�F�C�v�L�[�̒l
			m_shapeAnimations[shapeName].push_back(ShapeAnimation(time, value));
		}
	}

	//AnimationFrame�ɃV�F�C�v�L�[�̏�������
	for (AnimationFrame& frame : m_frames) {
		for (UINT i = 0; i < m_shapeNames.size(); i++) {
			ShapeAnimation* frameShapeValue = nullptr;
			ShapeAnimation* nextFrameShapeValue = nullptr;

			//m_shapeAnimations�ɕۑ����Ă���V�F�C�v�L�[�̃L�[�t���[�������ׂĎQ�� (�t���[�����ԏ��Ƀ\�[�g����Ă���)
			for (UINT j = 0; j < m_shapeAnimations[m_shapeNames[i]].size(); j++) {
				//�V�F�C�v�L�[�̃t���[�����Ԃ�AnimationFrame��菬�����ꍇ�͒l���X�V
				if (m_shapeAnimations[m_shapeNames[i]][j].time <= frame.time) {
					frameShapeValue = &m_shapeAnimations[m_shapeNames[i]][j];
				}
				else {
					//frameShapeValue�̎��̃t���[����ۑ�
					nextFrameShapeValue = &m_shapeAnimations[m_shapeNames[i]][j];
					break;
				}
			}

			if (!frameShapeValue) {
				continue;
			}

			float lerp = frameShapeValue->value;

			//���̃t���[�������݂��Ă���Ύ��̃t���[���ƍ��̃t���[���̊Ԃ���
			if (nextFrameShapeValue) {
				//Lerp�pt (0.0f�`1.0f)
				float t = (frame.time - frameShapeValue->time) / (nextFrameShapeValue->time - frameShapeValue->time);

				lerp = std::lerp(frameShapeValue->value, nextFrameShapeValue->value, t);
			}

			//��Ԃ����l������
			frame.shapeAnimations.push_back(lerp);
		}
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

	//Lerp�pt (0.0f�`1.0f)
	float t = (nowAnimTime - currentFrame->time) / (nextFrame->time - currentFrame->time);

	//���݂̃t���[���Ǝ��̃t���[���̊Ԃ���
	m_pTempFrame = new AnimationFrame(nowAnimTime);

	//�{�[���̕��
	for (UINT i = 0; i < currentFrame->boneAnimations.size(); i++) {
		BoneAnimation lerpBoneAnim{};
		lerpBoneAnim.position = Lerp(currentFrame->boneAnimations[i].position, nextFrame->boneAnimations[i].position, t);
		lerpBoneAnim.rotation = Lerp(currentFrame->boneAnimations[i].rotation, nextFrame->boneAnimations[i].rotation, t);
		m_pTempFrame->boneAnimations.push_back(lerpBoneAnim);
	}

	//�V�F�C�v�L�[�̕��
	for (UINT i = 0; i < currentFrame->shapeAnimations.size(); i++) {
		float shapeValue = std::lerp(currentFrame->shapeAnimations[i], nextFrame->shapeAnimations[i], t);
		m_pTempFrame->shapeAnimations.push_back(shapeValue);
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

AnimationFrame::AnimationFrame(float time)
	: time(time)
{
}
