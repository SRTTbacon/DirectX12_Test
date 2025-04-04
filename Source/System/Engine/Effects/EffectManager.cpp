#include "EffectManager.h"

using namespace DirectX;

EffectManager::EffectManager()
	: m_effekseerManager(nullptr)
	, m_effekseerRenderer(nullptr)
	, m_pCommandList(nullptr)
	, m_pCamera(nullptr)
{
}

EffectManager::~EffectManager()
{
}

void EffectManager::Initialize(ID3D12Device* pDevice, ID3D12CommandQueue* pCommandQueue, ID3D12GraphicsCommandList* pCommandList, Camera* pCamera, UINT bufferCount, int maxEffectCount)
{
    //レンダラーの生成
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_effekseerRenderer = EffekseerRendererDX12::Create(pDevice, pCommandQueue, bufferCount, &format, 1, DXGI_FORMAT_D32_FLOAT, false, 8000);

    //マネージャの生成
    m_effekseerManager = Effekseer::Manager::Create(maxEffectCount); //最大エフェクト数

	//メモリプールの作成
	m_efkMemoryPool = EffekseerRenderer::CreateSingleFrameMemoryPool(m_effekseerRenderer->GetGraphicsDevice());

	//コマンドリストの作成
	m_efkCommandList = EffekseerRenderer::CreateCommandList(m_effekseerRenderer->GetGraphicsDevice(), m_efkMemoryPool);

	//描画モジュールの設定
	m_effekseerManager->SetSpriteRenderer(m_effekseerRenderer->CreateSpriteRenderer());
	m_effekseerManager->SetRibbonRenderer(m_effekseerRenderer->CreateRibbonRenderer());
	m_effekseerManager->SetRingRenderer(m_effekseerRenderer->CreateRingRenderer());
	m_effekseerManager->SetTrackRenderer(m_effekseerRenderer->CreateTrackRenderer());
	m_effekseerManager->SetModelRenderer(m_effekseerRenderer->CreateModelRenderer());

	//テクスチャ、モデル、カーブ、マテリアルローダーの設定する。
	//ユーザーが独自で拡張できる。現在はファイルから読み込んでいる。
	m_effekseerManager->SetTextureLoader(m_effekseerRenderer->CreateTextureLoader());
	m_effekseerManager->SetModelLoader(m_effekseerRenderer->CreateModelLoader());
	m_effekseerManager->SetMaterialLoader(m_effekseerRenderer->CreateMaterialLoader());
	m_effekseerManager->SetCurveLoader(Effekseer::MakeRefPtr<Effekseer::CurveLoader>());

	m_pCommandList = pCommandList;
	m_pCamera = pCamera;
}

void EffectManager::LateUpdate()
{
	if (!m_pCamera || !m_effekseerManager.Get()) {
		return;
	}
	for (std::unique_ptr<Effect>& effect : m_effects) {
		effect->Update();
	}

	//Release()が実行されていれば解放
	std::erase_if(m_effects, [](const std::unique_ptr<Effect>& effect) {
		return effect->GetIsReleased();
		});

	// レイヤーパラメータの設定
	Effekseer::Manager::LayerParameter layerParameter;
	layerParameter.ViewerPosition = Effekseer::Vector3D(XMVectorGetX(m_pCamera->m_eyePos), XMVectorGetY(m_pCamera->m_eyePos), XMVectorGetZ(m_pCamera->m_eyePos));
	m_effekseerManager->SetLayerParameter(0, layerParameter);

	m_effekseerRenderer->SetProjectionMatrix(ConvertMatrix(m_pCamera->m_projMatrix));
	m_effekseerRenderer->SetCameraMatrix(ConvertMatrix(m_pCamera->m_viewMatrix));

	Effekseer::Manager::UpdateParameter updateParameter;
	m_effekseerManager->Update(updateParameter);
}

void EffectManager::BeginRender()
{
	if (!m_effekseerRenderer.Get()) {
		return;
	}

	m_efkMemoryPool->NewFrame();

	//コマンドリストを開始する。
	EffekseerRendererDX12::BeginCommandList(m_efkCommandList, m_pCommandList);
	m_effekseerRenderer->SetCommandList(m_efkCommandList);

	m_effekseerRenderer->BeginRendering();
}

void EffectManager::EndRender()
{
	if (!m_effekseerRenderer.Get()) {
		return;
	}

	m_effekseerRenderer->EndRendering();
	
	m_effekseerRenderer->SetCommandList(nullptr);
	EffekseerRendererDX12::EndCommandList(m_efkCommandList);
}

void EffectManager::Draw()
{
	if (!m_pCamera || !m_effekseerManager.Get()) {
		return;
	}

	Effekseer::Manager::DrawParameter drawParameter;
	m_effekseerManager->Draw(drawParameter);
}

Effect* EffectManager::CreateEffect(std::string path)
{
	if (!m_effekseerManager.Get()) {
		return nullptr;
	}

	std::u16string uPath = GetU16String(path);
	Effekseer::EffectRef effectRef = Effekseer::Effect::Create(m_effekseerManager, uPath.c_str());

	if (!effectRef.Get()) {
		printf("エフェクトを作成できませんでした。\n");
		return nullptr;
	}

	Effect* pEffect = AddEffect(effectRef);
	return pEffect;
}

Effect* EffectManager::CreateEffect(const void* data, int size)
{
	if (!m_effekseerManager.Get()) {
		return nullptr;
	}

	Effekseer::EffectRef effectRef = Effekseer::Effect::Create(m_effekseerManager, data, size);

	if (!effectRef.Get()) {
		printf("エフェクトを作成できませんでした。\n");
		return nullptr;
	}

	Effect* pEffect = AddEffect(effectRef);
	return pEffect;
}

Effekseer::Matrix44 EffectManager::ConvertMatrix(DirectX::XMMATRIX& mat)
{
	Effekseer::Matrix44 result;
	DirectX::XMFLOAT4X4 temp;
	DirectX::XMStoreFloat4x4(&temp, mat);

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result.Values[i][j] = temp.m[i][j];
		}
	}

	return result;
}

Effect* EffectManager::AddEffect(Effekseer::EffectRef effect)
{
	std::unique_ptr<Effect> effectHandle = std::make_unique<Effect>(m_effekseerManager, effect, &m_pCamera->m_eyePos);
	Effect* rawPtr = effectHandle.get();
	m_effects.push_back(std::move(effectHandle));

	return rawPtr;
}
