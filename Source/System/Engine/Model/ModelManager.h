#pragma once
#include "Model.h"
#include <queue>

class ModelManager
{
public:
	void AddModel(Model* pModel);

	//�X�V
	//�[�x����`�揇�Ԃ����肷�邽�߁A�e�t���[����Update�֐��̍Ō�Ɏ��s
	void Update(UINT backBufferIndex);

	//�[�x���傫�����ɉe��`��
	void RenderShadowMap(UINT backBufferIndex);

	//�[�x���傫�����ɖ{�̂�`��
	void RenderModel(UINT backBufferIndex);

private:
	// ��r�֐�: �[�x�̑傫�����ɕ��ׂ�
	struct DepthComparator {
		bool operator()(const std::pair<float, Model*>& a, const std::pair<float, Model*>& b) {
			return a.first < b.first;  //�[�x���傫�����Ƀ\�[�g
		}
	};

	//�[�x���ɕ��ׂ�D��x�t���L���[���쐬
	std::priority_queue<std::pair<float, Model*>, std::vector<std::pair<float, Model*>>, DepthComparator> m_opaqueModels;
	std::priority_queue<std::pair<float, Model*>, std::vector<std::pair<float, Model*>>, DepthComparator> m_transparentModels;
	std::vector<Model*> m_sortedOpaqueModels;
	std::vector<Model*> m_sortedTransparentModels;

	std::vector<Model*> m_models;
};