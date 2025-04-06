//���f�����œK�ȏ��Ԃŕ`�悳����N���X
//���̃N���X�́A��{�I�ɃG���W���ŊǗ����邽�ߌĂяo���Ȃ�

#pragma once

#include "Model.h"
#include <queue>

class ModelManager
{
public:
	~ModelManager();

	//�`�悳���郂�f����ǉ�
	void AddModel(Model* pModel);

	//���f�����폜
	void ReleaseModel(Model* pModel);

	//�X�V
	//�[�x����`�揇�Ԃ����肷�邽�߁A�e�t���[����Update�֐��̍Ō�Ɏ��s
	void LateUpdate(UINT backBufferIndex);

	//�[�x���傫�����ɉe��`��
	void RenderShadowMap(UINT backBufferIndex, bool bRenderShadow = true);

	//�[�x���傫�����ɖ{�̂�`��
	void RenderModel(ID3D12GraphicsCommandList* pCommandList, UINT backBufferIndex);

private:
	//��r�֐�: �[�x�̑傫�����ɕ��ׂ�
	struct DepthComparator {
		bool operator()(const std::pair<float, Mesh*>& a, const std::pair<float, Mesh*>& b) {
			return a.first < b.first;  //�[�x���傫�����Ƀ\�[�g
		}
	};

	std::unordered_map<Material*, std::vector<Mesh*>> m_opaqueMaterials;
	//�[�x���ɕ��ׂ�D��x�t���L���[���쐬
	std::priority_queue<std::pair<float, Mesh*>, std::vector<std::pair<float, Mesh*>>, DepthComparator> m_transparentModels;
	std::vector<Mesh*> m_sortedTransparentModels;

	std::vector<Model*> m_models;
};