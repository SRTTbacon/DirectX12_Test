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
	printf("boneCount = %u\n", boneCount);
	for (USHORT i = 0; i < boneCount; i++) {

		std::string boneName = br.ReadBytes(br.ReadByte());	//ボーン名
		m_boneMapping.push_back(boneName);
	}
	USHORT animCount = br.ReadUInt16();		//アニメーションのフレーム数
	printf("animCount = %u\n", animCount);

	for (int i = 0; i < animCount; i++) {
		float time = br.ReadFloat();			//フレーム時間
		AnimationFrame frame(time);
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
			frame.boneAnimations.push_back(bone);
		}

		m_frames.push_back(frame);
	}

	USHORT shapeCount = br.ReadUInt16();	//シェイプキーの数
	printf("shapeCount = %u\n", shapeCount);

	for (int i = 0; i < shapeCount; i++) {
		std::string shapeName = UTF8ToShiftJIS(br.ReadBytes(br.ReadByte()));
		m_shapeNames.push_back(shapeName);		//シェイプキーの名前を保存
		m_shapeAnimations[shapeName] = std::vector<ShapeAnimation>();

		USHORT frameCount = br.ReadUInt16();	//そのシェイプキーのアニメーション数

		for (int j = 0; j < frameCount; j++) {
			float time = br.ReadFloat();		//キーの時間
			float value = br.ReadFloat();		//シェイプキーの値
			m_shapeAnimations[shapeName].push_back(ShapeAnimation(time, value));
		}
	}

	//AnimationFrameにシェイプキーの情報を入れる
	for (AnimationFrame& frame : m_frames) {
		for (UINT i = 0; i < m_shapeNames.size(); i++) {
			ShapeAnimation* frameShapeValue = nullptr;
			ShapeAnimation* nextFrameShapeValue = nullptr;

			//m_shapeAnimationsに保存しているシェイプキーのキーフレームをすべて参照 (フレーム時間順にソートされている)
			for (UINT j = 0; j < m_shapeAnimations[m_shapeNames[i]].size(); j++) {
				//シェイプキーのフレーム時間がAnimationFrameより小さい場合は値を更新
				if (m_shapeAnimations[m_shapeNames[i]][j].time <= frame.time) {
					frameShapeValue = &m_shapeAnimations[m_shapeNames[i]][j];
				}
				else {
					//frameShapeValueの次のフレームを保存
					nextFrameShapeValue = &m_shapeAnimations[m_shapeNames[i]][j];
					break;
				}
			}

			if (!frameShapeValue) {
				continue;
			}

			float lerp = frameShapeValue->value;

			//次のフレームが存在していれば次のフレームと今のフレームの間を補間
			if (nextFrameShapeValue) {
				//Lerp用t (0.0f～1.0f)
				float t = (frame.time - frameShapeValue->time) / (nextFrameShapeValue->time - frameShapeValue->time);

				lerp = std::lerp(frameShapeValue->value, nextFrameShapeValue->value, t);
			}

			//補間した値を入れる
			frame.shapeAnimations.push_back(lerp);
		}
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

	//Lerp用t (0.0f～1.0f)
	float t = (nowAnimTime - currentFrame->time) / (nextFrame->time - currentFrame->time);

	//現在のフレームと次のフレームの間を補間
	m_pTempFrame = new AnimationFrame(nowAnimTime);

	//ボーンの補間
	for (UINT i = 0; i < currentFrame->boneAnimations.size(); i++) {
		BoneAnimation lerpBoneAnim{};
		lerpBoneAnim.position = Lerp(currentFrame->boneAnimations[i].position, nextFrame->boneAnimations[i].position, t);
		lerpBoneAnim.rotation = Lerp(currentFrame->boneAnimations[i].rotation, nextFrame->boneAnimations[i].rotation, t);
		m_pTempFrame->boneAnimations.push_back(lerpBoneAnim);
	}

	//シェイプキーの補間
	for (UINT i = 0; i < currentFrame->shapeAnimations.size(); i++) {
		float shapeValue = std::lerp(currentFrame->shapeAnimations[i], nextFrame->shapeAnimations[i], t);
		m_pTempFrame->shapeAnimations.push_back(shapeValue);
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

AnimationFrame::AnimationFrame(float time)
	: time(time)
{
}
