#include "Animation.h"

Animation::Animation(std::string animFilePath)
{
	BinaryReader br(animFilePath);
	USHORT boneCount = br.ReadUInt16();
	for (int i = 0; i < boneCount; i++) {
		std::string boneName = br.ReadBytes(br.ReadByte());
		float rotX = br.ReadFloat();
		float rotY = br.ReadFloat();
		float rotZ = br.ReadFloat();
		float posX = br.ReadFloat();
		float posY = br.ReadFloat();
		float posZ = br.ReadFloat();
		float initPosX = br.ReadFloat();
		float initPosY = br.ReadFloat();
		float initPosZ = br.ReadFloat();
		BoneAnimation bone{ boneName, DirectX::XMFLOAT3(posX, posY, posZ), DirectX::XMFLOAT3(initPosX, initPosY, initPosZ), DirectX::XMFLOAT3(rotX, rotY, rotZ) };
		m_boneAnim.push_back(bone);
	}
	//br.Close();
}
