#include "ModelManager.h"

void ModelManager::AddModel(Model* pModel)
{
	m_models.push_back(pModel);
}

void ModelManager::Update()
{
    //各オブジェクトの描画順番を計算
    for (Model* pModel : m_models) {
        //描画しない場合はスルー
        if (!pModel->m_bVisible) {
            continue;
        }

        //モデルの更新
        pModel->Update();

        //深度を取得
        float depth = pModel->GetZBuffer();
        //不透明か半透明かで分ける
        if (pModel->GetIsTransparent()) {
            m_transparentModels.push({ depth, pModel });
        }
        else {
            m_opaqueModels.push({ depth, pModel });
        }
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
    //本体を描画
    //先に不透明のモデルを描画し、次に半透明のモデルを描画
    for (Model* pModel : m_sortedOpaqueModels) {
        pModel->RenderSceneWithShadow();
    }

    for (Model* pModel : m_sortedTransparentModels) {
        pModel->RenderSceneWithShadow();
    }
}
