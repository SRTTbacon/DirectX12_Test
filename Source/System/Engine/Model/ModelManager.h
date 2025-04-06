//モデルを最適な順番で描画させるクラス
//このクラスは、基本的にエンジンで管理するため呼び出さない

#pragma once

#include "Model.h"
#include <queue>

class ModelManager
{
public:
	~ModelManager();

	//描画させるモデルを追加
	void AddModel(Model* pModel);

	//モデルを削除
	void ReleaseModel(Model* pModel);

	//更新
	//深度から描画順番を決定するため、各フレームのUpdate関数の最後に実行
	void LateUpdate(UINT backBufferIndex);

	//深度が大きい順に影を描画
	void RenderShadowMap(UINT backBufferIndex, bool bRenderShadow = true);

	//深度が大きい順に本体を描画
	void RenderModel(ID3D12GraphicsCommandList* pCommandList, UINT backBufferIndex);

private:
	//比較関数: 深度の大きい順に並べる
	struct DepthComparator {
		bool operator()(const std::pair<float, Mesh*>& a, const std::pair<float, Mesh*>& b) {
			return a.first < b.first;  //深度が大きい順にソート
		}
	};

	std::unordered_map<Material*, std::vector<Mesh*>> m_opaqueMaterials;
	//深度順に並べる優先度付きキューを作成
	std::priority_queue<std::pair<float, Mesh*>, std::vector<std::pair<float, Mesh*>>, DepthComparator> m_transparentModels;
	std::vector<Mesh*> m_sortedTransparentModels;

	std::vector<Model*> m_models;
};