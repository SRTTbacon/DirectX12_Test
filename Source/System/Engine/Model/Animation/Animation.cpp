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

//ファイルからアニメーションをロード
void Animation::Load(std::string animFilePath)
{
	//バイナリとして開く
	BinaryReader br(animFilePath);
	USHORT boneCount = br.ReadUInt16();		//アニメーションするボーン数
	printf("boneCount = %d\n", boneCount);
	for (USHORT i = 0; i < boneCount; i++) {

		std::string boneName = br.ReadBytes(br.ReadByte());	//ボーン名
		boneMapping.push_back(boneName);
	}
	USHORT animCount = br.ReadUInt16();		//アニメーションのフレーム数
	printf("animCount = %d\n", animCount);

	for (int i = 0; i < animCount; i++) {
		float time = br.ReadFloat();			//フレーム時間
		AnimationFrame frame{ time };
		for (int j = 0; j < boneCount; j++) {
			//回転
			float rotX = br.ReadFloat();
			float rotY = br.ReadFloat();
			float rotZ = br.ReadFloat();
			float rotW = br.ReadFloat();
			//相対位置
			float posX = br.ReadFloat();
			float posY = br.ReadFloat();
			float posZ = br.ReadFloat();
			BoneAnimation bone{ DirectX::XMFLOAT3(posX, posY, posZ), DirectX::XMFLOAT4(rotX, rotY, rotZ, rotW) };

			//配列に追加
			frame.animations.push_back(bone);
		}

		m_frames.push_back(frame);
	}
}

//指定したアニメーション時間のフレームを取得
AnimationFrame* Animation::GetFrame(float nowAnimTime)
{
	//補間用のフレームがあれば削除
	if (m_pTempFrame) {
		delete m_pTempFrame;
		m_pTempFrame = nullptr;
	}

	AnimationFrame* currentFrame = nullptr;
	AnimationFrame* nextFrame = nullptr;

	//nowAnimTimeの直前のフレームを取得
	for (std::vector<AnimationFrame>::iterator it = m_frames.begin(); it != m_frames.end(); it++) {
		if (it->time <= nowAnimTime) {
			currentFrame = &(*it);

			//次のフレームがあれば取得
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

	//フレームが存在しなければ終了
	if (currentFrame == nullptr) {
		return nullptr;
	}
	//次のフレームが存在しなければ最後のフレームをそのまま返す
	if (!nextFrame) {
		return currentFrame;
	}

	//Lerp用 (0.0f～1.0f)
	float t = (nowAnimTime - currentFrame->time) / (nextFrame->time - currentFrame->time);

	//現在のフレームと次のフレームの間を補間
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

//フレームが最後のフレームかどうか
bool Animation::IsLastFrame(AnimationFrame* pAnimFrame)
{
	if (!pAnimFrame)
		return false;
	return pAnimFrame == &m_frames[m_frames.size() - 1];
}
