#include "ModelManager.h"

void ModelManager::AddModel(Model* pModel)
{
	m_models.push_back(pModel);
}

void ModelManager::LateUpdate(UINT backBufferIndex)
{
    //�e�I�u�W�F�N�g�̕`�揇�Ԃ��v�Z
    for (Model* pModel : m_models) {
        //�`�悵�Ȃ��ꍇ�̓X���[
        if (!pModel->m_bVisible) {
            continue;
        }

        //���f���̍X�V
        pModel->Update(backBufferIndex);

        //�[�x���擾
        float depth = pModel->GetZBuffer();
        //�s���������������ŕ�����
        if (pModel->GetIsTransparent()) {
            m_transparentModels.push({ depth, pModel });
        }
        else {
            m_opaqueModels.push({ depth, pModel });
        }
    }
}

void ModelManager::RenderShadowMap(UINT backBufferIndex)
{
    m_sortedOpaqueModels.clear();
    m_sortedTransparentModels.clear();

    //�[�x���傫�����Ƀ��f�������o���ĕ`��
    while (!m_opaqueModels.empty()) {
        Model* pModel = m_opaqueModels.top().second;
        m_opaqueModels.pop();

        pModel->RenderShadowMap(backBufferIndex);

        m_sortedOpaqueModels.push_back(pModel);
    }

    while (!m_transparentModels.empty()) {
        Model* pModel = m_transparentModels.top().second;
        m_transparentModels.pop();

        pModel->RenderShadowMap(backBufferIndex);

        m_sortedTransparentModels.push_back(pModel);
    }
}

void ModelManager::RenderModel(UINT backBufferIndex)
{
    //�{�̂�`��
    //��ɕs�����̃��f����`�悵�A���ɔ������̃��f����`��
    for (Model* pModel : m_sortedOpaqueModels) {
        pModel->RenderSceneWithShadow(backBufferIndex);
    }

    for (Model* pModel : m_sortedTransparentModels) {
        pModel->RenderSceneWithShadow(backBufferIndex);
    }
}
