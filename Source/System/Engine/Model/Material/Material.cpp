#include "Material.h"

using namespace std;

Material::Material(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, DirectionalLight* pDirectionalLight, DescriptorHeap* pDescriptorHeap, UINT materialID)
	: m_pDevice(pDevice)
	, m_pCommandList(pCommandList)
	, m_pDirectionalLight(pDirectionalLight)
	, m_pDescriptorHeap(pDescriptorHeap)
	, m_materialID(materialID)
	, m_shaderKind(ShaderKinds::PrimitiveShader)
	, m_transparentValue(1.0f)
	, m_bTransparent(false)
	, m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_pMainTexture(nullptr)
	, m_pNormalTexture(nullptr)
	, m_psBufferResource(nullptr)
	, m_pPSBufferMap(nullptr)
{
}

Material::~Material()
{
	if (m_psBufferResource) {
		m_psBufferResource->Unmap(0, nullptr);
	}

	if (m_pMainTexture) {
		delete m_pMainTexture;
		m_pMainTexture = nullptr;
	}
	if (m_pNormalTexture) {
		delete m_pNormalTexture;
		m_pNormalTexture = nullptr;
	}
}

void Material::Initialize(ID3D12Resource* pDefaultMainTex, ID3D12Resource* pDefaultNormalTex)
{
	m_pDescriptorHeap->SetMainTexture(m_materialID, pDefaultMainTex, pDefaultNormalTex, nullptr);
	m_psBufferResource = m_pDirectionalLight->CreateConstantBuffer();
	HRESULT hr = m_psBufferResource->Map(0, nullptr, &m_pPSBufferMap);
	if (FAILED(hr)) {
		printf("バッファのマップに失敗しました。エラーコード:%d\n", hr);
	}
}

void Material::SetPipeline(RootSignature* pRootSignature, PipelineState* pPipelineState, ShaderKinds shaderKind)
{
	m_pRootSignature = pRootSignature;
	m_pPipelineState = pPipelineState;
	m_shaderKind = shaderKind;
}

void Material::SetMainTexture(string texPath)
{
	if (m_pMainTexture) {
		delete m_pMainTexture;
	}

	m_pMainTexture = Texture2D::Get(texPath);

	m_pDescriptorHeap->SetMainTexture(m_materialID, m_pMainTexture->Resource(), nullptr, nullptr);
}

void Material::SetNormalMap(std::string texPath)
{
	if (m_pNormalTexture) {
		delete m_pNormalTexture;
	}

	m_pNormalTexture = Texture2D::Get(texPath);

	m_pDescriptorHeap->SetMainTexture(m_materialID, nullptr, m_pNormalTexture->Resource(), nullptr);
}

void Material::SetShapeData(UINT index, ID3D12Resource* pShapeTexture)
{
	m_pDescriptorHeap->SetResource(index, pShapeTexture, SHAPE_FORMAT);
}

void Material::SetIsTransparent(bool bTransparent)
{
	m_bTransparent = bTransparent;
}

void Material::SetOpacity(float value)
{
	m_transparentValue = value;
}

D3D12_GPU_DESCRIPTOR_HANDLE Material::GetShapeData(UINT offset)
{
	return m_pDescriptorHeap->GetGpuDescriptorHandle(offset);
}

bool Material::GetIsTransparent() const
{
	return m_bTransparent;
}

void Material::ExecutePipeline()
{
	m_pCommandList->SetGraphicsRootSignature(m_pRootSignature->GetRootSignature());   //ルートシグネチャを設定
	m_pCommandList->SetPipelineState(m_pPipelineState->GetPipelineState());           //パイプラインステートを設定

	if (m_bTransparent) {
		m_pDirectionalLight->m_lightBuffer.cameraEyePos.w = m_transparentValue;
	}
	else {
		m_pDirectionalLight->m_lightBuffer.cameraEyePos.w = 1.0f;
	}
	m_pDirectionalLight->MemCopyBuffer(m_pPSBufferMap);

	m_pCommandList->SetGraphicsRootConstantBufferView(2, m_psBufferResource->GetGPUVirtualAddress());	//ディレクショナルライトの情報を送信
	m_pCommandList->SetGraphicsRootDescriptorTable(3, m_pDescriptorHeap->GetGpuDescriptorHandle(0));     //シャドウマップ
	m_pCommandList->SetGraphicsRootDescriptorTable(4, m_pDescriptorHeap->GetGpuDescriptorHandle(m_materialID, 0));			//ピクセルシェーダのテクスチャ
}

void Material::ExecuteShapeData(UINT index)
{
	if (m_shaderKind == ShaderKinds::BoneShader) {
		m_pCommandList->SetGraphicsRootDescriptorTable(5, m_pDescriptorHeap->GetGpuDescriptorHandle(index));		//頂点シェーダーのシェイプキー
	}
}
