#include "ModelManager.h"

ModelManager::~ModelManager()
{
    for (Model* pModel : m_models) {
        delete pModel;
    }

    m_models.clear();
}

void ModelManager::AddModel(Model* pModel)
{
	m_models.push_back(pModel);
}

void ModelManager::ReleaseModel(Model* pModel)
{
    for (auto it = m_models.begin(); it != m_models.end(); it++) {
        if (*it == pModel) {
            m_models.erase(it);
            delete pModel;
            break;
        }
    }
}

void ModelManager::LateUpdate(UINT backBufferIndex)
{
    m_opaqueMaterials.clear();

    //各オブジェクトの描画順番を計算
    for (Model* pModel : m_models) {
        //描画しない場合はスルー
        if (!pModel->m_bVisible) {
            continue;
        }

        //モデルの更新
        pModel->LateUpdate(backBufferIndex);

        //深度を取得
        float depth = pModel->GetZBuffer();

        for (UINT i = 0; i < pModel->GetMeshCount(); i++) {
            Mesh* pMesh = pModel->GetMesh(i);
            //不透明か半透明かで分ける
            if (pMesh->pMaterial->GetIsTransparent()) {
                m_transparentModels.push({ depth, pMesh });
            }
            else {
                m_opaqueMaterials[pMesh->pMaterial].push_back(pMesh);
            }
        }
    }
}

void ModelManager::RenderShadowMap(UINT backBufferIndex, bool bRenderShadow)
{
    m_sortedTransparentModels.clear();

    for (auto& keyValue : m_opaqueMaterials) {
        std::vector<Mesh*>& pMeshes = keyValue.second;
        for (Mesh* pMesh : pMeshes) {
            if (bRenderShadow && !pMesh->pModel->GetIsShadowRendered()) {
                pMesh->pModel->RenderShadowMap(backBufferIndex);
            }
        }
    }

    //深度が大きい順にモデルを取り出して描画
    while (!m_transparentModels.empty()) {
        Mesh* pMesh = m_transparentModels.top().second;
        m_transparentModels.pop();

        if (bRenderShadow && !pMesh->pModel->GetIsShadowRendered())
            pMesh->pModel->RenderShadowMap(backBufferIndex);

        m_sortedTransparentModels.push_back(pMesh);
    }
}

void ModelManager::RenderModel(ID3D12GraphicsCommandList* pCommandList, UINT backBufferIndex)
{
    //本体を描画
    //先に不透明のモデルを描画し、次に半透明のモデルを描画
    for (auto& keyValue : m_opaqueMaterials) {
        std::vector<Mesh*>& pMeshes = keyValue.second;
        keyValue.first->ExecutePipeline();
        for (Mesh* pMesh : pMeshes) {
            keyValue.first->ExecuteShapeData(pMesh->meshData->shapeDataIndex);
            if (pMesh->meshData->shapeDataIndex > 0) {
                keyValue.first->SetShapeData(pMesh->meshData->shapeDataIndex, pMesh->meshData->shapeDeltasBuffer.Get());
            }
            pMesh->DrawMesh(pCommandList, backBufferIndex, pMesh->pModel->m_modelConstantStruct.modelMatrix);
        }
    }

    for (Mesh* pMesh : m_sortedTransparentModels) {
        pMesh->pMaterial->ExecutePipeline();
        if (pMesh->meshData->shapeDataIndex > 0) {
            pMesh->pMaterial->SetShapeData(pMesh->meshData->shapeDataIndex, pMesh->meshData->shapeDeltasBuffer.Get());
        }
        pMesh->pMaterial->ExecuteShapeData(pMesh->meshData->shapeDataIndex);
        pMesh->DrawMesh(pCommandList, backBufferIndex, pMesh->pModel->m_modelConstantStruct.modelMatrix);
    }
}
