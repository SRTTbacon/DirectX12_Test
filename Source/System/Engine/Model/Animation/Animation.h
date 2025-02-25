//(�{�[��)�A�j���[�V���������t�@�C��(.hsc)����ǂݍ���

#pragma once
#include "..\\..\\Core\\BinaryFile\\BinaryCompression.h"
#include <DirectXMath.h>
#include <vector>
#include <unordered_map>
#include "..\\..\\..\\Main\\Utility.h"

//�A�j���[�V�����t�@�C���̃w�b�_�[
constexpr const char* ANIMATION_HEADER = "HCSAnim";

//�{�[���̈ʒu�A��]
struct BoneAnimation
{
	DirectX::XMFLOAT3 position;	//�ʒu
	DirectX::XMFLOAT4 rotation;	//��]
};
//�V�F�C�v�L�[
struct ShapeAnimation
{
	float time;		//�t���[������
	float value;	//�V�F�C�v�L�[�̒l
};
//�t���[��
struct AnimationFrame
{
	float time;		//�t���[������
	BoneAnimation armatureAnimation;
	std::vector<BoneAnimation> boneAnimations;		//�e�{�[���̈ʒu�A��]
	std::vector<float> shapeAnimations;				//�e�V�F�C�v�L�[�̒l(0.0f�`1.0f)

	AnimationFrame(float time);
};

//�A�j���[�V����
class Animation
{
public:
	Animation();
	~Animation();

	//.hsc�t�@�C������A�j���[�V���������[�h
	void Load(std::string animFilePath);

	//�w�肵���A�j���[�V�������Ԃ̃t���[�����擾
	AnimationFrame* GetFrame(float nowAnimTime);
	//�t���[�����Ō�̃t���[�����ǂ���
	bool IsLastFrame(AnimationFrame* pAnimFrame);

	//�{�[�����ꗗ
	std::vector<std::string> m_boneMapping;
	//�V�F�C�v�L�[���ꗗ
	std::vector<std::string> m_shapeNames;
	//�A�j���[�V�����̃t���[���ꗗ
	std::vector<AnimationFrame> m_frames;

public:
	inline std::string GetFilePath() const { return m_animFilePath; }

private:
	//�t���[����ԗp
	AnimationFrame* m_pTempFrame;

	std::unordered_map<std::string, std::vector<ShapeAnimation>> m_shapeAnimations;

	std::string m_animFilePath;
};