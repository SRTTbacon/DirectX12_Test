//(ボーン)アニメーション情報をファイル(.hsc)から読み込む

#pragma once
#include "..\\..\\Core\\BinaryFile\\BinaryCompression.h"
#include <DirectXMath.h>
#include <vector>
#include <unordered_map>
#include "..\\..\\..\\Main\\Utility.h"

//アニメーションファイルのヘッダー
constexpr const char* ANIMATION_HEADER = "HCSAnim";

//ボーンの位置、回転
struct BoneAnimation
{
	DirectX::XMFLOAT3 position;	//位置
	DirectX::XMFLOAT4 rotation;	//回転
};
//シェイプキー
struct ShapeAnimation
{
	float time;		//フレーム時間
	float value;	//シェイプキーの値
};
//フレーム
struct AnimationFrame
{
	float time;		//フレーム時間
	BoneAnimation armatureAnimation;
	std::vector<BoneAnimation> boneAnimations;		//各ボーンの位置、回転
	std::vector<float> shapeAnimations;				//各シェイプキーの値(0.0f～1.0f)

	AnimationFrame(float time);
};

//アニメーション
class Animation
{
public:
	Animation();
	~Animation();

	//.hscファイルからアニメーションをロード
	void Load(std::string animFilePath);

	//指定したアニメーション時間のフレームを取得
	AnimationFrame* GetFrame(float nowAnimTime);
	//フレームが最後のフレームかどうか
	bool IsLastFrame(AnimationFrame* pAnimFrame);

	//ボーン名一覧
	std::vector<std::string> m_boneMapping;
	//シェイプキー名一覧
	std::vector<std::string> m_shapeNames;
	//アニメーションのフレーム一覧
	std::vector<AnimationFrame> m_frames;

public:
	inline std::string GetFilePath() const { return m_animFilePath; }

private:
	//フレーム補間用
	AnimationFrame* m_pTempFrame;

	std::unordered_map<std::string, std::vector<ShapeAnimation>> m_shapeAnimations;

	std::string m_animFilePath;
};