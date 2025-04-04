#pragma once
#include <Effekseer.h>
#include <DirectXMath.h>

#include "..\\Core\\XMFLOATHelper.h"

class EffectHandle
{
public:
	friend class Effect;

	EffectHandle(const Effekseer::ManagerRef effekseerManager, const Effekseer::Handle handle, const DirectX::XMFLOAT3* pParentScale);

	//再開
	void Play();
	//一時停止
	void Pause();
	//エフェクトを停止し破棄する。この関数以降はEffectHandleクラスは使用不可
	void Stop();

	//再生速度を変更 (0.0～1.0f～n)
	void SetSpeed(float speed) const;

	void SetScale(float scale);

public:
	//現在の再生速度を取得
	float GetSpeed() const;

	//ワールドスケールを取得
	DirectX::XMFLOAT3 GetWorldScale() const;

	//エフェクトが存在しているか
	inline bool GetIsAlive() const { return m_bAlive; }

public:
	//エフェクトの位置
	DirectX::XMFLOAT3 m_position;
	//このエフェクトのローカルスケール
	DirectX::XMFLOAT3 m_localScale;
	bool m_bDraw;

private:
	const Effekseer::ManagerRef m_effekseerManager;
	const Effekseer::Handle m_handle;

	//エフェクト全体のスケール
	const DirectX::XMFLOAT3* m_pParentScale;

	bool m_bAlive;		//使用可能なエフェクトか
	bool m_bPlaying;	//再生中か

	bool GetIsExistEffect() const;

	//位置やスケールを更新
	//Effectクラスから実行される
	void Update();
};

class Effect
{
public:
	Effect(Effekseer::ManagerRef effekseerManager, Effekseer::EffectRef effect, DirectX::XMVECTOR* pCameraPos);

	//エフェクトを再生
	//@param XMFLOAT3 初期位置
	//@param bool 同じエフェクトを削除してから再生
	//@return 再生中のエフェクトのハンドル
	EffectHandle* Play(DirectX::XMFLOAT3 position, bool bOnceFalse = false);

	//エフェクト全体のスケールを設定
	//@param float スケール (x, y, zに同じ値が入る)
	void SetScale(float scale);
	//エフェクト全体のスケールを設定
	//@param XMFLOAT3 スケール
	void SetScale(DirectX::XMFLOAT3 scale);

	//描画距離を設定
	//カメラとの距離はこの値以上になると描画されなくなる
	//param float 距離
	void SetHiddenDistance(float distancePow);

	//再生が終了したハンドルを解放
	void Update();

	void Release();

public:
	//描画距離を取得
	inline float GetHiddenDistance() const { return m_hiddenDistance; }

	inline bool GetIsReleased() const { return m_bReleased; }

private:
	Effekseer::ManagerRef m_effekseerManager;
	Effekseer::EffectRef m_effect;

	std::vector<std::unique_ptr<EffectHandle>> m_handles;	//このタイプのすべてのエフェクトを管理

	DirectX::XMVECTOR* m_pCameraPos;
	DirectX::XMFLOAT3 m_scale;

	//表示距離
	float m_hiddenDistance;

	bool m_bReleased;
};
