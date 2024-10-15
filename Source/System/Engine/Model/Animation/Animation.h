#pragma once
#include "..\\..\\Core\\BinaryFile\\BinaryReader.h"
#include <DirectXMath.h>
#include <vector>
#include "..\\..\\..\\Main\\Utility.h"

//ボーンごとの位置、回転
struct BoneAnimation
{
	DirectX::XMFLOAT3 position;	//位置
	DirectX::XMFLOAT4 rotation;	//回転
};
//フレーム
struct AnimationFrame
{
	float time;		//フレーム時間
	std::vector<BoneAnimation> animations;	//各ボーンの位置、回転
};

//アニメーション
class Animation
{
public:
	Animation();
	~Animation();

	//ファイルからアニメーションをロード
	void Load(std::string animFilePath);

	//指定したアニメーション時間のフレームを取得
	AnimationFrame* GetFrame(float nowAnimTime);
	//フレームが最後のフレームかどうか
	bool IsLastFrame(AnimationFrame* pAnimFrame);

	//アニメーションが入っているボーン名一覧
	std::vector<std::string> boneMapping;
	//アニメーションのフレーム一覧
	std::vector<AnimationFrame> m_frames;

private:
	//フレーム補間用
	AnimationFrame* m_pTempFrame;
};