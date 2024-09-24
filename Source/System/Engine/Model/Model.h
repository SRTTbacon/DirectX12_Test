#pragma once
#include "..\\Engine.h"

#include "..\\Core\\SharedStruct\\SharedStruct.h"
#include "..\\Core\\VertexBuffer\\VertexBuffer.h"
#include "..\\Core\\RootSignature\\RootSignature.h"
#include "..\\Core\\PipelineState\\PipelineState.h"
#include "..\\\Core\\IndexBuffer\\IndexBuffer.h"
#include "..\\Core\\FBXLoader\\FBXLoader.h"
#include "..\\Core\\DescriptorHeap\\DescriptorHeap.h"
#include "..\\Core\\Texture2D\\Texture2D.h"

#include "..\\Camera\\Camera.h"

class Model
{
public:
	void Initialize(const wchar_t* fileName, Camera* camera, bool bCharacter);

	void Update();
	void Draw();

	void SetPosition(DirectX::XMFLOAT3 pos);

	Model();

private:
	DescriptorHeap* descriptorHeap;										//GPU�ɑ��郊�\�[�X�𑝂₷����
	std::vector<DescriptorHandle*> materialHandles;						//�e�N�X�`���p�̃n���h���ꗗ(FBX���̃}�e���A�����Ɠ���)

	VertexBuffer* vertexBuffer;											//���_�o�b�t�@(���_�̔z��BGPU�ɒ��_�f�[�^�𑗂邽�ߖ��t���[������������R�s�[���邩��d���B���ǂ�����)
	RootSignature* rootSignature;										//���[�g�V�O�l�`��(���炭GPU�ɑ���f�[�^���Ǘ�������)
	PipelineState* pipelineState;										//�p�C�v���C��(�`����@�B�V�F�[�_�[�̐ݒ�Ȃ�)
	IndexBuffer* indexBuffer;											//�C���f�b�N�X�o�b�t�@(���_�ɔԍ���t������)

	Camera* m_camera;													//�J�����̃|�C���^
	ConstantBuffer* m_constantBuffer[Engine::FRAME_BUFFER_COUNT];		//�R���X�^���g�o�b�t�@(�J��������ǂ������邩)�B���f���̂�����h�~�̂���2�p�ӂ���

	DirectX::XMFLOAT3 m_pos;
	DirectX::XMFLOAT3 m_rot;

	std::vector<VertexBuffer*> vertexBuffers;							//���b�V���̐����̒��_�o�b�t�@
	std::vector<IndexBuffer*> indexBuffers;								//���b�V���̐����̃C���f�b�N�X�o�b�t�@

	std::vector<Mesh> meshes;											//���b�V��

	FBXLoader loader;

	float m_animTime;
};