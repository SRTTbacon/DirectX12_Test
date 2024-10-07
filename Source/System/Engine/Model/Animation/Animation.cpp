#include "Animation.h"

Animation::Animation(std::string animFilePath)
	: m_nowAnimTime(0.0f)
{
	BinaryReader br(animFilePath);
	USHORT boneCount = br.ReadUInt16();
	printf("boneCount = %d\n", boneCount);
	for (USHORT i = 0; i < boneCount; i++) {

		std::string boneName = br.ReadBytes(br.ReadByte());
		boneMapping.push_back(boneName);
	}
	USHORT animCount = br.ReadUInt16();
	printf("animCount = %d\n", animCount);
	for (int i = 0; i < animCount; i++) {
		float time = br.ReadFloat();
		AnimationFrame frame{ time };
		for (int j = 0; j < boneCount; j++) {
			float rotX = br.ReadFloat();
			float rotY = br.ReadFloat();
			float rotZ = br.ReadFloat();
			float rotW = br.ReadFloat();
			float posX = br.ReadFloat();
			float posY = br.ReadFloat();
			float posZ = br.ReadFloat();
			BoneAnimation bone{ DirectX::XMFLOAT3(posX, posY, posZ), DirectX::XMFLOAT4(rotX, rotY, rotZ, rotW) };
			frame.anim.push_back(bone);
		}
		m_frames.push_back(frame);
	}
	//br.Close();
}

std::vector<BoneAnimation>& Animation::Update()
{
	m_nowAnimTime += g_Engine->GetFrameTime();

	AnimationFrame* currentFrame = nullptr;
	for (AnimationFrame& frame : m_frames) {
		if (frame.time <= m_nowAnimTime) {
			currentFrame = &frame;
		}
		else {
			break;
		}
	}

	if (currentFrame == nullptr) {
		std::vector<BoneAnimation> temp;
		return temp;
	}

	if (currentFrame == &m_frames[m_frames.size() - 1]) {
		m_nowAnimTime = 0.0f;
	}

	return currentFrame->anim;
}
