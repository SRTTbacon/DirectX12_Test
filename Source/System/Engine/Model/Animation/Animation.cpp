#include "Animation.h"

void Animation::Load(std::string animFilePath)
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
			frame.animations.push_back(bone);
		}
		m_frames.push_back(frame);
	}
	//br.Close();
}

AnimationFrame* Animation::GetFrame(float nowAnimTime)
{
	AnimationFrame* currentFrame = nullptr;
	for (AnimationFrame& frame : m_frames) {
		if (frame.time <= nowAnimTime) {
			currentFrame = &frame;
		}
		else {
			break;
		}
	}

	if (currentFrame == nullptr) {
		return nullptr;
	}

	return currentFrame;
}

bool Animation::IsLastFrame(AnimationFrame* pAnimFrame)
{
	if (!pAnimFrame)
		return false;
	return pAnimFrame == &m_frames[m_frames.size() - 1];
}
