#pragma once

#include <DirectXMath.h>
#include <vector>
#include <unordered_map>

#include "..\\..\\..\\Main\\Utility.h"
#include "..\\..\\Core\\BinaryFile\\BinaryCompression.h"

//(ボーン)アニメーション情報をファイル(.hsc)から読み込む

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
struct CharacterAnimationFrame
{
	float time;		//フレーム時間

	//ボーンアニメーション
	BoneAnimation armatureAnimation;
	std::vector<BoneAnimation> boneAnimations;		//各ボーンの位置、回転
	std::vector<float> shapeAnimations;				//各シェイプキーの値(0.0f～1.0f)

	CharacterAnimationFrame(float time);
};

//モデルの位置、回転またはスケール
struct ModelAnimation
{
	float nextFrameTime;
	float time;
	DirectX::XMFLOAT3 nextFrameValue;
	DirectX::XMFLOAT3 value;
};

//モデルアニメーションのフレーム
struct ModelAnimationFrame
{
	float time;

	//key : メッシュインデックス
	//value : 位置、回転、スケールの値
	std::unordered_map<UINT, DirectX::XMFLOAT3> position;
	std::unordered_map<UINT, DirectX::XMFLOAT3> rotation;
	std::unordered_map<UINT, DirectX::XMFLOAT3> scale;
};

//アニメーション
class Animation
{
public:
	enum FrameType
	{
		FrameType_Position,
		FrameType_Rotation,
		FrameType_Scale
	};

public:
	Animation();
	~Animation();

	//.hscファイルからアニメーションをロード (キャラクターのみ)
	void Load(std::string animFilePath);

	void SetMaxTime(float time);
	void SetAnimName(std::string name);

	//キーフレームを追加 (時間が小さい順にする必要あり)
	void AddFrame(FrameType frameType, UINT meshIndex, float time, DirectX::XMFLOAT3 value);

	//指定したアニメーション時間のフレームを取得
	//引数 : float フレーム時間(秒), UINT 1フレーム前のフレームインデックス(あれば処理速度上昇)
	CharacterAnimationFrame* GetCharacterFrame(float nowAnimTime, _In_opt_ UINT* pBeforeFrameIndex = nullptr);

	ModelAnimationFrame GetModelFrame(float nowAnimTime);

	//フレームが最後のフレームかどうか
	bool IsLastFrame(CharacterAnimationFrame* pAnimFrame);
	bool IsLastFrame(float time) const;

	//ボーン名一覧
	std::vector<std::string> m_boneMapping;
	//シェイプキー名一覧
	std::vector<std::string> m_shapeNames;
	//アニメーションのフレーム一覧
	std::vector<CharacterAnimationFrame> m_frames;

	//各位置、角度、スケールのアニメーション
	//key : メッシュインデックス
	std::unordered_map<UINT, std::vector<ModelAnimation>> m_positionFrames;
	std::unordered_map<UINT, std::vector<ModelAnimation>> m_rotationFrames;
	std::unordered_map<UINT, std::vector<ModelAnimation>> m_scaleFrames;

public:
	//アニメーションファイルを取得
	inline std::string GetFilePath() const { return m_animFilePath; }

	//アニメーション名
	inline std::string GetAnimName() const { return m_animName; }

	//アニメーション時間を取得
	inline float GetMaxTime() const;

private:
	//シェイプキーのアニメーション
	std::unordered_map<std::string, std::vector<ShapeAnimation>> m_shapeAnimations;

	//フレーム補間用
	CharacterAnimationFrame* m_pTempFrame;

	std::string m_animFilePath;
	std::string m_animName;

	float m_maxTime;

	void GetShapeFrame(float time, std::vector<float>& shapeWeights);
};
