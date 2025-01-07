//モデルを最適な順番で描画させるクラス
//このクラスは、基本的にエンジンで管理するため呼び出さない

#pragma once

#include "Model.h"
#include <queue>

class ModelManager
{
public:
	//描画させるモデルを追加
	void AddModel(Model* pModel);

	//更新
	//深度から描画順番を決定するため、各フレームのUpdate関数の最後に実行
	void LateUpdate(UINT backBufferIndex);

	//深度が大きい順に影を描画
	void RenderShadowMap(UINT backBufferIndex, bool bRenderShadow = true);

	//深度が大きい順に本体を描画
	void RenderModel(UINT backBufferIndex);

private:
	//比較関数: 深度の大きい順に並べる
	struct DepthComparator {
		bool operator()(const std::pair<float, Model*>& a, const std::pair<float, Model*>& b) {
			return a.first < b.first;  //深度が大きい順にソート
		}
	};

	//深度順に並べる優先度付きキューを作成
	std::priority_queue<std::pair<float, Model*>, std::vector<std::pair<float, Model*>>, DepthComparator> m_opaqueModels;
	std::priority_queue<std::pair<float, Model*>, std::vector<std::pair<float, Model*>>, DepthComparator> m_transparentModels;
	std::vector<Model*> m_sortedOpaqueModels;
	std::vector<Model*> m_sortedTransparentModels;

	std::vector<Model*> m_models;
};