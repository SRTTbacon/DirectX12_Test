#include "Effect.h"

using namespace DirectX;

Effect::Effect(const Effekseer::ManagerRef effekseerManager, const Effekseer::EffectRef effect, XMVECTOR* pCameraPos)
	: m_effekseerManager(effekseerManager)
	, m_effect(effect)
	, m_pCameraPos(pCameraPos)
	, m_scale(XMFLOAT3(1.0f, 1.0f, 1.0f))
	, m_hiddenDistance(10000.0f)
	, m_bReleased(false)
{
}

EffectHandle* Effect::Play(XMFLOAT3 position, bool bOnce)
{
	if (!m_effekseerManager.Get()) {
		return nullptr;
	}

	if (bOnce) {
		//再生済みのエフェクトをすべて削除
		for (std::unique_ptr<EffectHandle>& handle : m_handles) {
			m_effekseerManager->StopEffect(handle->m_handle);
		}
		m_handles.clear();
	}

	//再生
	Effekseer::Handle handle = m_effekseerManager->Play(m_effect, position.x, position.y, position.z);

	if (handle == -1) {
		printf("エフェクトを再生できませんでした。\n");
		return nullptr;
	}

	std::unique_ptr<EffectHandle> effectHandle = std::make_unique<EffectHandle>(m_effekseerManager, handle, &m_scale);
	EffectHandle* rawPtr = effectHandle.get();
	rawPtr->m_position = position;
	m_handles.push_back(std::move(effectHandle));

	return rawPtr;
}

void Effect::SetScale(float scale)
{
	SetScale(XMFLOAT3(scale, scale, scale));
}

void Effect::SetScale(DirectX::XMFLOAT3 scale)
{
	m_scale = scale;
}

void Effect::SetHiddenDistance(float distancePow)
{
	m_hiddenDistance = distancePow;
}

void Effect::Update()
{
	//エフェクトの再生が終了していたら解放
	std::erase_if(m_handles, [](const std::unique_ptr<EffectHandle>& handle) {
		if (!handle->GetIsExistEffect()) {
			handle->m_bAlive = false;
			handle->m_bPlaying = false;
			return true;
		}
		return false;
		});

	//ワールドスケールを更新
	for (std::unique_ptr<EffectHandle>& handle : m_handles) {
		handle->Update();

		//範囲外かどうか調べる
		XMFLOAT3 cameraPos;
		XMStoreFloat3(&cameraPos, *m_pCameraPos);
		float distance = Distance(handle->m_position, cameraPos);

		bool bDraw = distance < m_hiddenDistance && handle->m_bDraw;
		m_effekseerManager->SetShown(handle->m_handle, bDraw);
	}
}

void Effect::Release()
{
	m_bReleased = true;
}

EffectHandle::EffectHandle(Effekseer::ManagerRef effekseerManager, Effekseer::Handle handle, const DirectX::XMFLOAT3* pParentScale)
	: m_effekseerManager(effekseerManager)
	, m_handle(handle)
	, m_pParentScale(pParentScale)
	, m_position(XMFLOAT3(0.0f, 0.0f, 0.0f))
	, m_localScale(XMFLOAT3(1.0f, 1.0f, 1.0f))
	, m_bAlive(true)
	, m_bPlaying(true)
	, m_bDraw(true)
{
}

void EffectHandle::Play()
{
	if (!m_effekseerManager.Get() || !m_bAlive) {
		return;
	}

	m_effekseerManager->SetPaused(m_handle, false);
	m_bPlaying = true;
}

void EffectHandle::Pause()
{
	if (!m_effekseerManager.Get() || !m_bAlive) {
		return;
	}

	m_effekseerManager->SetPaused(m_handle, true);
	m_bPlaying = false;
}

void EffectHandle::Stop()
{
	if (!m_effekseerManager.Get() || !m_bAlive) {
		return;
	}

	m_effekseerManager->StopEffect(m_handle);

	m_bAlive = false;
	m_bPlaying = false;
}

void EffectHandle::SetSpeed(float speed) const
{
	if (!m_effekseerManager.Get() || !m_bAlive) {
		return;
	}

	m_effekseerManager->SetSpeed(m_handle, speed);
}

void EffectHandle::SetScale(float scale)
{
	m_localScale = XMFLOAT3(scale, scale, scale);
}

bool EffectHandle::GetIsExistEffect() const
{
	return m_effekseerManager->Exists(m_handle);
}

void EffectHandle::Update()
{
	if (!m_bAlive) {
		return;
	}

	XMFLOAT3 worldScale = *m_pParentScale * m_localScale;

	m_effekseerManager->SetScale(m_handle, worldScale.x, worldScale.y, worldScale.z);
	m_effekseerManager->SetLocation(m_handle, Effekseer::Vector3D(m_position.x, m_position.y, m_position.z));
}

float EffectHandle::GetSpeed() const
{
	if (!m_effekseerManager.Get() || !m_bAlive) {
		return 0.0f;
	}

	return m_effekseerManager->GetSpeed(m_handle);
}

XMFLOAT3 EffectHandle::GetWorldScale() const
{
	XMFLOAT3 worldScale = *m_pParentScale * m_localScale;

	return worldScale;
}
