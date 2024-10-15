#pragma once
#include "..\\..\\Core\\BinaryFile\\BinaryReader.h"
#include <DirectXMath.h>
#include <vector>
#include "..\\..\\..\\Main\\Utility.h"

//�{�[�����Ƃ̈ʒu�A��]
struct BoneAnimation
{
	DirectX::XMFLOAT3 position;	//�ʒu
	DirectX::XMFLOAT4 rotation;	//��]
};
//�t���[��
struct AnimationFrame
{
	float time;		//�t���[������
	std::vector<BoneAnimation> animations;	//�e�{�[���̈ʒu�A��]
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
	std::vector<std::string> boneMapping;
	//�A�j���[�V�����̃t���[���ꗗ
	std::vector<AnimationFrame> m_frames;

private:
	//�t���[����ԗp
	AnimationFrame* m_pTempFrame;
};