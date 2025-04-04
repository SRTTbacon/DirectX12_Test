#pragma once

#include <DirectXMath.h>
#include <vector>
#include <unordered_map>

#include "..\\..\\..\\Main\\Utility.h"
#include "..\\..\\Core\\BinaryFile\\BinaryCompression.h"

//(�{�[��)�A�j���[�V���������t�@�C��(.hsc)����ǂݍ���

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
struct CharacterAnimationFrame
{
	float time;		//�t���[������

	//�{�[���A�j���[�V����
	BoneAnimation armatureAnimation;
	std::vector<BoneAnimation> boneAnimations;		//�e�{�[���̈ʒu�A��]
	std::vector<float> shapeAnimations;				//�e�V�F�C�v�L�[�̒l(0.0f�`1.0f)

	CharacterAnimationFrame(float time);
};

//���f���̈ʒu�A��]�܂��̓X�P�[��
struct ModelAnimation
{
	float nextFrameTime;
	float time;
	DirectX::XMFLOAT3 nextFrameValue;
	DirectX::XMFLOAT3 value;
};

//���f���A�j���[�V�����̃t���[��
struct ModelAnimationFrame
{
	float time;

	//key : ���b�V���C���f�b�N�X
	//value : �ʒu�A��]�A�X�P�[���̒l
	std::unordered_map<UINT, DirectX::XMFLOAT3> position;
	std::unordered_map<UINT, DirectX::XMFLOAT3> rotation;
	std::unordered_map<UINT, DirectX::XMFLOAT3> scale;
};

//�A�j���[�V����
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

	//.hsc�t�@�C������A�j���[�V���������[�h (�L�����N�^�[�̂�)
	void Load(std::string animFilePath);

	void SetMaxTime(float time);
	void SetAnimName(std::string name);

	//�L�[�t���[����ǉ� (���Ԃ����������ɂ���K�v����)
	void AddFrame(FrameType frameType, UINT meshIndex, float time, DirectX::XMFLOAT3 value);

	//�w�肵���A�j���[�V�������Ԃ̃t���[�����擾
	//���� : float �t���[������(�b), UINT 1�t���[���O�̃t���[���C���f�b�N�X(����Ώ������x�㏸)
	CharacterAnimationFrame* GetCharacterFrame(float nowAnimTime, _In_opt_ UINT* pBeforeFrameIndex = nullptr);

	ModelAnimationFrame GetModelFrame(float nowAnimTime);

	//�t���[�����Ō�̃t���[�����ǂ���
	bool IsLastFrame(CharacterAnimationFrame* pAnimFrame);
	bool IsLastFrame(float time) const;

	//�{�[�����ꗗ
	std::vector<std::string> m_boneMapping;
	//�V�F�C�v�L�[���ꗗ
	std::vector<std::string> m_shapeNames;
	//�A�j���[�V�����̃t���[���ꗗ
	std::vector<CharacterAnimationFrame> m_frames;

	//�e�ʒu�A�p�x�A�X�P�[���̃A�j���[�V����
	//key : ���b�V���C���f�b�N�X
	std::unordered_map<UINT, std::vector<ModelAnimation>> m_positionFrames;
	std::unordered_map<UINT, std::vector<ModelAnimation>> m_rotationFrames;
	std::unordered_map<UINT, std::vector<ModelAnimation>> m_scaleFrames;

public:
	//�A�j���[�V�����t�@�C�����擾
	inline std::string GetFilePath() const { return m_animFilePath; }

	//�A�j���[�V������
	inline std::string GetAnimName() const { return m_animName; }

	//�A�j���[�V�������Ԃ��擾
	inline float GetMaxTime() const;

private:
	//�V�F�C�v�L�[�̃A�j���[�V����
	std::unordered_map<std::string, std::vector<ShapeAnimation>> m_shapeAnimations;

	//�t���[����ԗp
	CharacterAnimationFrame* m_pTempFrame;

	std::string m_animFilePath;
	std::string m_animName;

	float m_maxTime;

	void GetShapeFrame(float time, std::vector<float>& shapeWeights);
};
