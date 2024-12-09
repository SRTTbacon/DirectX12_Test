#pragma once
#include "..\\..\\Core\\BinaryFile\\BinaryReader.h"
#include <DirectXMath.h>
#include <vector>
#include <unordered_map>
#include "..\\..\\..\\Main\\Utility.h"

//�{�[�����Ƃ̈ʒu�A��]
struct BoneAnimation
{
	DirectX::XMFLOAT3 position;	//�ʒu
	DirectX::XMFLOAT4 rotation;	//��]
};
//�V�F�C�v�L�[
struct ShapeAnimation
{
	float time;
	float value;
};
//�t���[��
struct AnimationFrame
{
	float time;		//�t���[������
	std::vector<BoneAnimation> boneAnimations;		//�e�{�[���̈ʒu�A��]
	std::vector<float> shapeAnimations;	//�e�V�F�C�v�L�[�̒l

	AnimationFrame(float time);
};

//�A�j���[�V����
class Animation
{
public:
	Animation();
	~Animation();

	//�t�@�C������A�j���[�V���������[�h
	void Load(std::string animFilePath);

	//�w�肵���A�j���[�V�������Ԃ̃t���[�����擾
	AnimationFrame* GetFrame(float nowAnimTime);
	//�t���[�����Ō�̃t���[�����ǂ���
	bool IsLastFrame(AnimationFrame* pAnimFrame);

	//�A�j���[�V�����������Ă���{�[�����ꗗ
	std::vector<std::string> m_boneMapping;
	std::vector<std::string> m_shapeNames;
	//�A�j���[�V�����̃t���[���ꗗ
	std::vector<AnimationFrame> m_frames;

private:
	//�t���[����ԗp
	AnimationFrame* m_pTempFrame;

	std::unordered_map<std::string, std::vector<ShapeAnimation>> m_shapeAnimations;
};