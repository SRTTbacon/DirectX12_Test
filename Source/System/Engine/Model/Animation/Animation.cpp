#include "Animation.h"
#include "..\\..\\Core\\XMFLOATHelper.h"

#include <filesystem>

using namespace DirectX;

Animation::Animation()
	: m_pTempFrame(nullptr)
	, m_maxTime(0.0f)
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
	m_boneMapping.clear();
	m_shapeNames.clear();
	m_frames.clear();
	m_shapeAnimations.clear();

	if (m_pTempFrame) {
		delete m_pTempFrame;
		m_pTempFrame = nullptr;
	}

	DWORD startTime = timeGetTime();

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
	BinaryDecompressZSD(animBuffer, animBufferOriginalSize, compressedBuffer, animBufferCompressedSize);

	delete[] compressedBuffer;

	br.Close();

	DWORD temp2 = timeGetTime();

	br = BinaryReader(animBuffer);
	for (int i = 0; i < animCount; i++) {
		float time = br.ReadFloat();			//�t���[������

		CharacterAnimationFrame frame(time);

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

	m_animFilePath = animFilePath;

	std::filesystem::path path(animFilePath);
	m_animName = path.filename().string();;

	DWORD endTime = timeGetTime();
	printf("LoadAnimation - %dms\n", endTime - startTime);
}

void Animation::SetMaxTime(float time)
{
	m_maxTime = time;
}

void Animation::SetAnimName(std::string name)
{
	m_animName = name;
}

void Animation::AddFrame(FrameType frameType, UINT meshIndex, float time, DirectX::XMFLOAT3 value)
{
	if (frameType == FrameType::FrameType_Position) {
		m_positionFrames[meshIndex].push_back(ModelAnimation(-1.0f, time, XMFLOAT3(0.0f, 0.0f, 0.0f), value / 100.0f));
	}
	else if (frameType == FrameType::FrameType_Rotation) {
		m_rotationFrames[meshIndex].push_back(ModelAnimation(-1.0f, time, XMFLOAT3(0.0f, 0.0f, 0.0f), value));
	}
	else if (frameType == FrameType::FrameType_Scale) {
		m_scaleFrames[meshIndex].push_back(ModelAnimation(-1.0f, time, XMFLOAT3(0.0f, 0.0f, 0.0f), value / 100.0f));
	}
}

//�w�肵���A�j���[�V�������Ԃ̃t���[�����擾
CharacterAnimationFrame* Animation::GetCharacterFrame(float nowAnimTime, _In_opt_ UINT* pBeforeFrameIndex)
{
	if (nowAnimTime < 0.0f) {
		nowAnimTime = 0.0f;
	}

	//��ԗp�̃t���[��������΍폜
	if (m_pTempFrame) {
		delete m_pTempFrame;
		m_pTempFrame = nullptr;
	}

	UINT beforeFrameIndex = 0;
	if (pBeforeFrameIndex) {
		beforeFrameIndex = *pBeforeFrameIndex;
	}

	CharacterAnimationFrame* currentFrame = nullptr;
	CharacterAnimationFrame* nextFrame = nullptr;

	//�O��̃t���[�����Ԃ��nowAnimTime�����������0�Ԗڂ���Č���
	if (pBeforeFrameIndex && m_frames[*pBeforeFrameIndex].time > nowAnimTime) {
		*pBeforeFrameIndex = 0;
	}

	//nowAnimTime�̒��O�̃t���[�����擾
	for (int i = static_cast<UINT>(beforeFrameIndex); i < static_cast<int>(m_frames.size()); i++) {
		CharacterAnimationFrame& frame = m_frames[i];
		beforeFrameIndex = static_cast<UINT>(i);
		if (frame.time <= nowAnimTime) {
			currentFrame = &frame;

			//���̃t���[��������Ύ擾
			UINT nextIndex = i + 1;
			if (nextIndex < static_cast<UINT>(m_frames.size())) {
				nextFrame = &m_frames[nextIndex];
				if (nextFrame->time > nowAnimTime) {
					break;
				}
			}
			else {
				nextFrame = nullptr;
				break;
			}
		}
		else {
			beforeFrameIndex = -1;
		}
	}

	//�t���[�������݂��Ȃ���ΏI��
	if (currentFrame == nullptr) {
		return nullptr;
	}

	if (pBeforeFrameIndex) {
		*pBeforeFrameIndex = beforeFrameIndex;
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
	m_pTempFrame = new CharacterAnimationFrame(nowAnimTime);

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

	GetShapeFrame(nowAnimTime, m_pTempFrame->shapeAnimations);

	return m_pTempFrame;
}

ModelAnimationFrame Animation::GetModelFrame(float nowAnimTime)
{
	if (nowAnimTime < 0.0f) {
		nowAnimTime = 0.0f;
	}

	std::unordered_map<UINT, ModelAnimation> pCurrentPositionFrame;

	std::unordered_map<UINT, ModelAnimation> pCurrentRotationFrame;

	std::unordered_map<UINT, ModelAnimation> pCurrentScaleFrame;

	for (std::pair<UINT, std::vector<ModelAnimation>> pair : m_positionFrames) {
		for (size_t i = 0; i < pair.second.size(); i++) {
			ModelAnimation& anim = pair.second[i];
			pCurrentPositionFrame.emplace(pair.first, ModelAnimation());
			pCurrentPositionFrame[pair.first].time = -1.0f;
			if (anim.time <= nowAnimTime) {
				pCurrentPositionFrame[pair.first] = anim;
				pCurrentPositionFrame[pair.first].nextFrameTime = -1.0f;
				if (i + 1 < pair.second.size()) {
					if (pair.second[i + 1].time > nowAnimTime) {
						pCurrentPositionFrame[pair.first].nextFrameTime = pair.second[i + 1].time;
						pCurrentPositionFrame[pair.first].nextFrameValue = pair.second[i + 1].value;
						break;
					}
				}
				else {
					pCurrentPositionFrame[pair.first].nextFrameTime = m_maxTime;
					pCurrentPositionFrame[pair.first].nextFrameValue = pair.second[0].value;
					break;
				}
			}
		}
	}
	for (std::pair<UINT, std::vector<ModelAnimation>> pair : m_rotationFrames) {
		for (size_t i = 0; i < pair.second.size(); i++) {
			ModelAnimation& anim = pair.second[i];
			pCurrentRotationFrame.emplace(pair.first, ModelAnimation());
			pCurrentRotationFrame[pair.first].time = -1.0f;
			if (anim.time <= nowAnimTime) {
				pCurrentRotationFrame[pair.first] = anim;
				pCurrentRotationFrame[pair.first].nextFrameTime = -1.0f;
				if (i + 1 < pair.second.size()) {
					if (pair.second[i + 1].time > nowAnimTime) {
						pCurrentRotationFrame[pair.first].nextFrameTime = pair.second[i + 1].time;
						pCurrentRotationFrame[pair.first].nextFrameValue = pair.second[i + 1].value;
						break;
					}
				}
				else {
					pCurrentRotationFrame[pair.first].nextFrameTime = m_maxTime;
					pCurrentRotationFrame[pair.first].nextFrameValue = pair.second[0].value;
					break;
				}
			}
		}
	}
	for (std::pair<UINT, std::vector<ModelAnimation>> pair : m_scaleFrames) {
		for (size_t i = 0; i < pair.second.size(); i++) {
			ModelAnimation& anim = pair.second[i];
			pCurrentScaleFrame.emplace(pair.first, ModelAnimation());
			pCurrentScaleFrame[pair.first].time = -1.0f;
			if (anim.time <= nowAnimTime) {
				pCurrentScaleFrame[pair.first] = anim;
				pCurrentScaleFrame[pair.first].nextFrameTime = -1.0f;
				if (i + 1 < pair.second.size()) {
					if (pair.second[i + 1].time > nowAnimTime) {
						pCurrentScaleFrame[pair.first].nextFrameTime = m_maxTime;
						pCurrentScaleFrame[pair.first].nextFrameValue = pair.second[0].value;
						break;
					}
				}
				else {
					break;
				}
			}
		}
	}

	ModelAnimationFrame frame{};
	frame.time = nowAnimTime;

	for (std::pair<UINT, ModelAnimation> pair : pCurrentPositionFrame) {
		if (pair.second.time >= 0.0f) {
			if (pair.second.nextFrameTime >= 0.0f) {
				float t = (nowAnimTime - pair.second.time) / (pair.second.nextFrameTime - pair.second.time);
				t = min(1.0f, t);
				t = max(0.0f, t);
				frame.position.emplace(pair.first, Lerp(pair.second.value, pair.second.nextFrameValue, t));
			}
			else {
				frame.position.emplace(pair.first, pair.second.value);
			}
		}
	}
	for (std::pair<UINT, ModelAnimation> pair : pCurrentRotationFrame) {
		if (pair.second.time >= 0.0f) {
			if (pair.second.nextFrameTime >= 0.0f) {
				float t = (nowAnimTime - pair.second.time) / (pair.second.nextFrameTime - pair.second.time);
				t = min(1.0f, t);
				t = max(0.0f, t);
				frame.rotation.emplace(pair.first, Lerp(pair.second.value, pair.second.nextFrameValue, t));
			}
			else {
				frame.rotation.emplace(pair.first, pair.second.value);
			}
		}
	}
	for (std::pair<UINT, ModelAnimation> pair : pCurrentScaleFrame) {
		if (pair.second.time >= 0.0f) {
			if (pair.second.nextFrameTime >= 0.0f) {
				float t = (nowAnimTime - pair.second.time) / (pair.second.nextFrameTime - pair.second.time);
				t = min(1.0f, t);
				t = max(0.0f, t);
				frame.scale.emplace(pair.first, Lerp(pair.second.value, pair.second.nextFrameValue, t));
			}
			else {
				frame.scale.emplace(pair.first, pair.second.value);
			}
		}
	}

	return frame;
}

//�t���[�����Ō�̃t���[�����ǂ���
bool Animation::IsLastFrame(CharacterAnimationFrame* pAnimFrame)
{
	if (!pAnimFrame)
		return false;
	return pAnimFrame == &m_frames[m_frames.size() - 1];
}

bool Animation::IsLastFrame(float time) const
{
	if (m_maxTime > 0.0f) {
		return m_maxTime <= time;
	}

	return GetMaxTime() <= time;
}

inline float Animation::GetMaxTime() const
{
	if (m_maxTime > 0.0f) {
		return m_maxTime;
	}

	float maxTime = 0.0f;
	if (m_frames.size() > 0){
		if (maxTime < m_frames[m_frames.size() - 1].time) {
			maxTime = m_frames[m_frames.size() - 1].time;
		}
	}

	return maxTime;
}

void Animation::GetShapeFrame(float time, std::vector<float>& shapeWeights)
{
	shapeWeights.clear();
	for (UINT i = 0; i < m_shapeNames.size(); i++) {
		ShapeAnimation* frameShapeValue = nullptr;
		ShapeAnimation* nextFrameShapeValue = nullptr;

		//m_shapeAnimations�ɕۑ����Ă���V�F�C�v�L�[�̃L�[�t���[�������ׂĎQ�� (�t���[�����ԏ��Ƀ\�[�g����Ă���)
		for (UINT j = 0; j < m_shapeAnimations[m_shapeNames[i]].size(); j++) {
			//�V�F�C�v�L�[�̃t���[�����Ԃ�time��菬�����ꍇ�͒l���X�V
			if (m_shapeAnimations[m_shapeNames[i]][j].time <= time) {
				frameShapeValue = &m_shapeAnimations[m_shapeNames[i]][j];
			}
			else {
				//frameShapeValue�̎��̃t���[����ۑ�
				nextFrameShapeValue = &m_shapeAnimations[m_shapeNames[i]][j];
				break;
			}
		}

		if (!frameShapeValue || !nextFrameShapeValue) {
			shapeWeights.push_back(0.0f);
			continue;
		}

		float lerp = frameShapeValue->value;

		//���̃t���[�������݂��Ă���Ύ��̃t���[���ƍ��̃t���[���̊Ԃ���
		if (nextFrameShapeValue) {
			//Lerp�pt (0.0f�`1.0f)
			float t = (time - frameShapeValue->time) / (nextFrameShapeValue->time - frameShapeValue->time);

			lerp = std::lerp(frameShapeValue->value, nextFrameShapeValue->value, t);
		}

		//��Ԃ����l������
		shapeWeights.push_back(lerp);
	}
}

CharacterAnimationFrame::CharacterAnimationFrame(float time)
	: time(time)
	, armatureAnimation(BoneAnimation())
{
}
