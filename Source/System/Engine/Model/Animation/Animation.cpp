#include "Animation.h"

using namespace DirectX;

Animation::Animation()
	: m_pTempFrame(nullptr)
	, m_beforeFrameIndex(0)
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
	char* headerBuf = br.ReadBytes(br.ReadByte());
	std::string header = headerBuf;
	delete[] headerBuf;
	if (header != ANIMATION_HEADER) {
		br.Close();
		return;
	}

	USHORT boneCount = br.ReadUInt16();		//�A�j���[�V��������{�[����
	for (USHORT i = 0; i < boneCount; i++) {

		char* boneNameBuf = br.ReadBytes(br.ReadByte());
		std::string boneName = boneNameBuf;	//�{�[����
		delete[] boneNameBuf;
		m_boneMapping.push_back(boneName);
	}
	USHORT animCount = br.ReadUInt16();		//�A�j���[�V�����̃t���[����

	//�A�j���[�V�����f�[�^�͈��k����Ă��邽�߉�
	UINT animBufferOriginalSize = br.ReadUInt32();
	UINT animBufferCompressedSize = br.ReadUInt32();
	char* compressedBuffer = br.ReadBytes(animBufferCompressedSize);

	//���k����Ă���{�[��������
	std::vector<char> animBuffer;
	BinaryDecompress(animBuffer, animBufferOriginalSize, compressedBuffer, animBufferCompressedSize);

	delete[] compressedBuffer;

	br.Close();
	br = BinaryReader(animBuffer);
	for (int i = 0; i < animCount; i++) {
		float time = br.ReadFloat();			//�t���[������

		AnimationFrame frame(time);

		//��]
		float rotX = br.ReadFloat();
		float rotY = br.ReadFloat();
		float rotZ = br.ReadFloat();
		float rotW = br.ReadFloat();
		//�����ʒu����̑��Έʒu
		float posX = br.ReadFloat();
		float posY = br.ReadFloat();
		float posZ = br.ReadFloat();
		BoneAnimation armatureBone{ XMFLOAT3(posX, posY, posZ), XMFLOAT4(rotX, rotY, rotZ, rotW) };
		frame.armatureAnimation = armatureBone;

		for (int j = 0; j < boneCount; j++) {
			//��]
			rotX = br.ReadFloat();
			rotY = br.ReadFloat();
			rotZ = br.ReadFloat();
			rotW = br.ReadFloat();
			//�����ʒu����̑��Έʒu
			posX = br.ReadFloat();
			posY = br.ReadFloat();
			posZ = -br.ReadFloat();
			BoneAnimation bone{ XMFLOAT3(posX, posY, posZ), XMFLOAT4(rotX, rotY, rotZ, rotW) };

			//�z��ɒǉ�
			frame.boneAnimations.push_back(bone);
		}

		m_frames.push_back(frame);
	}

	USHORT shapeCount = br.ReadUInt16();	//�V�F�C�v�L�[�̐�

	for (int i = 0; i < shapeCount; i++) {
		//�V�F�C�v�L�[�̖��O�͓��{�ꂪ�܂܂�Ă��邱�Ƃ����邽��ShiftJIS�Ŏ擾 (�؍���Ƃ������ꂪ�܂܂�Ă���ƕ�������)
		char* shapeNameBuf = br.ReadBytes(br.ReadByte());
		std::string shapeName = UTF8ToShiftJIS(shapeNameBuf);
		delete[] shapeNameBuf;

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

	m_animFilePath = animFilePath;
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

	//�O��̃t���[�����Ԃ��nowAnimTime�����������0�Ԗڂ���Č���
	if (m_frames[m_beforeFrameIndex].time > nowAnimTime) {
		m_beforeFrameIndex = 0;
	}

	//nowAnimTime�̒��O�̃t���[�����擾
	for (UINT i = m_beforeFrameIndex; i < static_cast<UINT>(m_frames.size()); i++) {
		AnimationFrame& frame = m_frames[i];
		if (frame.time <= nowAnimTime) {
			currentFrame = &frame;

			//���̃t���[��������Ύ擾
			UINT nextIndex = i + 1;
			if (nextIndex < static_cast<UINT>(m_frames.size())) {
				nextFrame = &m_frames[nextIndex];
			}
			else {
				nextFrame = nullptr;
				break;
			}
		}
		else {
			m_beforeFrameIndex = i - 1;
			if (m_beforeFrameIndex < 0) {
				m_beforeFrameIndex = 0;
			}

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
	t = min(1.0f, t);
	t = max(0.0f, t);

	//���݂̃t���[���Ǝ��̃t���[���̊Ԃ���
	m_pTempFrame = new AnimationFrame(nowAnimTime);

	BoneAnimation lerpArmatureBoneAnim{};
	lerpArmatureBoneAnim.position = Lerp(currentFrame->armatureAnimation.position, nextFrame->armatureAnimation.position, t);
	lerpArmatureBoneAnim.rotation = Lerp(currentFrame->armatureAnimation.rotation, nextFrame->armatureAnimation.rotation, t);
	m_pTempFrame->armatureAnimation = lerpArmatureBoneAnim;

	//�{�[���̕�� (Linear)
	for (UINT i = 0; i < currentFrame->boneAnimations.size(); i++) {
		BoneAnimation lerpBoneAnim{};
		lerpBoneAnim.position = Lerp(currentFrame->boneAnimations[i].position, nextFrame->boneAnimations[i].position, t);
		lerpBoneAnim.rotation = Lerp(currentFrame->boneAnimations[i].rotation, nextFrame->boneAnimations[i].rotation, t);
		m_pTempFrame->boneAnimations.push_back(lerpBoneAnim);
	}

	//�V�F�C�v�L�[�̕�� (Linear)
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
	, armatureAnimation(BoneAnimation())
{
}
