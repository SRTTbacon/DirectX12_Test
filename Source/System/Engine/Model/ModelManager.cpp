#include "ModelManager.h"

void ModelManager::AddModel(Model* pModel)
{
	m_models.push_back(pModel);
}

void ModelManager::Update()
{
    //各オブジェクトの描画順番を計算
    for (Model* pModel : m_models) {
        float depth = pModel->GetZBuffer();
        if (pModel->GetIsTransparent()) {
            m_transparentModels.push({ depth, pModel });
        }
        else {
            m_opaqueModels.push({ depth, pModel });
        }

        pModel->Update();
    }
}

void ModelManager::RenderShadowMap()
{
    m_sortedOpaqueModels.clear();
    m_sortedTransparentModels.clear();

    //深度が大きい順にモデルを取り出して描画
    while (!m_opaqueModels.empty()) {
        Model* pModel = m_opaqueModels.top().second;
        m_opaqueModels.pop();

        pModel->RenderShadowMap();

        m_sortedOpaqueModels.push_back(pModel);
    }

    while (!m_transparentModels.empty()) {
        Model* pModel = m_transparentModels.top().second;
        m_transparentModels.pop();

        pModel->RenderShadowMap();

        m_sortedTransparentModels.push_back(pModel);
    }
}

void ModelManager::RenderModel()
{
    for (Model* pModel : m_sortedOpaqueModels) {
        pModel->RenderSceneWithShadow();
    }

    for (Model* pModel : m_sortedTransparentModels) {
        pModel->RenderSceneWithShadow();
    }
}
