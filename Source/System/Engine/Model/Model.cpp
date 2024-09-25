#include "Model.h"
#include "..\\Core\\FBXLoader\\FBXLoader.h"
#include "..\\Core\\DescriptorHeap\\DescriptorHeap.h"
#include "..\\Core\\Texture2D\\Texture2D.h"

using namespace DirectX;

// 拡張子を置き換える処理
#include <filesystem>
namespace fs = std::filesystem;
static std::wstring ReplaceExtension(const std::wstring& origin, const char* ext)
{
	fs::path p = origin.c_str();
	return p.replace_extension(ext).c_str();
}

void Model::Initialize(const wchar_t* fileName, Camera* camera, bool bCharacter)
{
	m_camera = camera;

	ImportSettings importSetting = ImportSettings
	{
		fileName,
		meshes,
		false,
		true, //テクスチャのUVのVだけ反転してるっぽい？ので読み込み時にUV座標を逆転させる
		bCharacter
	};

	if (!loader.Load(importSetting))
	{
		printf("FBXのロードに失敗\n");
		return;
	}

	printf("FBXをロードしました。\n");

	//メッシュの数だけ頂点バッファを用意する
	vertexBuffers.reserve(meshes.size());
	for (size_t i = 0; i < meshes.size(); i++)
	{
		auto size = sizeof(Vertex) * meshes[i].Vertices.size();
		auto stride = sizeof(Vertex);
		auto vertices = meshes[i].Vertices.data();
		for (size_t j = 0; j < meshes[i].Vertices.size(); j++)
			vertices[j].Position.x += 0.5f;
		auto pVB = new VertexBuffer(size, stride, vertices);
		if (!pVB->IsValid())
		{
			printf("頂点バッファの生成に失敗\n");
			return;
		}

		vertexBuffers.push_back(pVB);
	}

	//メッシュの数だけインデックスバッファを用意する
	indexBuffers.reserve(meshes.size());
	for (size_t i = 0; i < meshes.size(); i++)
	{
		auto size = sizeof(uint32_t) * meshes[i].Indices.size();
		auto indices = meshes[i].Indices.data();
		auto pIB = new IndexBuffer(size, indices);
		if (!pIB->IsValid())
		{
			printf("インデックスバッファの生成に失敗\n");
			return;
		}

		indexBuffers.push_back(pIB);
	}

	// マテリアルの読み込み
	descriptorHeap = new DescriptorHeap();
	materialHandles.clear();
	for (size_t i = 0; i < meshes.size(); i++)
	{
		auto texPath = ReplaceExtension(meshes[i].DiffuseMap, "png"); //
		auto mainTex = Texture2D::Get(texPath);
		auto handle = descriptorHeap->Register(mainTex);
		//printf("Texture = %s\n", FBXLoader::ToUTF8(texPath).c_str());
		materialHandles.push_back(handle);
	}

	rootSignature = new RootSignature(g_Engine->Device());
	/*if (!rootSignature->IsValid())
	{
		printf("ルートシグネチャの生成に失敗\n");
		return;
	}*/

	pipelineState = new PipelineState(g_Engine->Device());
	pipelineState->SetVS(L"x64/Debug/SimpleVS.cso");
	pipelineState->SetPS(L"x64/Debug/SimplePS.cso");
	pipelineState->CreatePipelineState(rootSignature);
	if (!pipelineState->IsValid())
	{
		printf("パイプラインステートの生成に失敗\n");
		return;
	}

	m_pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_rot = XMFLOAT3(-90.0f, 0.0f, 0.0f);

	m_animTime = 0.0f;
}

void Model::Update()
{
	Transform* ptr1 = m_camera->m_constantBuffer;
	Transform* ptr2 = m_constantBuffer[g_Engine->CurrentBackBufferIndex()]->GetPtr<Transform>();

	XMMATRIX matScale = XMMatrixScaling(10.0f, 10.0f, 10.0f);
	XMMATRIX matPos = XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);
	XMMATRIX matRot = XMMatrixRotationX(m_rot.x);
	matRot *= XMMatrixRotationY(m_rot.y);
	matRot *= XMMatrixRotationZ(m_rot.z);

	ptr2->World = matScale * matRot * matPos;
	ptr2->View = ptr1->View;
	ptr2->Proj = ptr1->Proj;
	for (int i = 0; i < MAX_BONES; i++)
		ptr2->BoneTransforms[i] = loader.boneInfos[i].finalTransformation;

	m_animTime += 0.016f;

	/*for (size_t i = 0; i < loader.boneInfos.size(); ++i) {
		// ボーンの位置、回転、スケールを計算
		DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f); // 例として原点
		DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationY(m_animTime); // Y軸周りに回転
		DirectX::XMMATRIX scaling = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f); // スケールはそのまま

		// 最終トランスフォームを計算
		loader.boneInfos[i].finalTransformation = scaling * rotation * translation * loader.boneInfos[i].offset;

		// アニメーションのスピードや時間に応じて変化を加えることができます
	}*/
}

void Model::Draw()
{
	UINT currentBufferIndex = g_Engine->CurrentBackBufferIndex();
	auto commandList = g_Engine->CommandList();
	auto materialHeap = descriptorHeap->GetHeap(); // ディスクリプタヒープ

	for (size_t i = 0; i < meshes.size(); i++)
	{
		auto vbView = vertexBuffers[i]->View();
		auto ibView = indexBuffers[i]->View();
		commandList->SetGraphicsRootSignature(rootSignature->Get());
		commandList->SetPipelineState(pipelineState->Get());

		// 頂点バッファの設定
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
		vertexBufferView.BufferLocation = vbView.BufferLocation; // vertexBufferは事前に作成しておく必要があります
		vertexBufferView.StrideInBytes = sizeof(Vertex); // Vertex構造体のサイズ
		vertexBufferView.SizeInBytes = sizeof(Vertex) * (UINT)meshes[i].Vertices.size(); // 頂点数

		commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
		//commandList->IASetVertexBuffers(0, 1, &vbView);

		// インデックスバッファの設定
		D3D12_INDEX_BUFFER_VIEW indexBufferView{};
		indexBufferView.BufferLocation = ibView.BufferLocation; // indexBufferは事前に作成しておく必要があります
		indexBufferView.Format = DXGI_FORMAT_R16_UINT; // インデックス形式（16ビット）
		indexBufferView.SizeInBytes = sizeof(uint16_t) * (UINT)meshes[i].Indices.size(); // インデックス数

		//commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer[currentBufferIndex]->GetAddress());

		commandList->IASetIndexBuffer(&indexBufferView);
		//commandList->IASetIndexBuffer(&ibView);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// ボーンのトランスフォームをシェーダーに送る
		for (size_t i = 0; i < loader.boneInfos.size(); ++i) {
			// ボーンの最終トランスフォームを送信（ボーン数が多い場合、バッファを使用することを検討）
			commandList->SetGraphicsRoot32BitConstants(1, 16, &loader.boneInfos[i].finalTransformation, 0);
		}

		commandList->SetDescriptorHeaps(1, &materialHeap); // 使用するディスクリプタヒープをセット
		commandList->SetGraphicsRootDescriptorTable(1, materialHandles[i]->HandleGPU); // そのメッシュに対応するディスクリプタテーブルをセット

		commandList->DrawIndexedInstanced((UINT)meshes[i].Indices.size(), 1, 0, 0, 0);
	}
}

void Model::SetPosition(DirectX::XMFLOAT3 pos)
{
	m_pos = pos;
}

Model::Model()
{
	for (int i = 0; i < Engine::FRAME_BUFFER_COUNT; i++) {
		m_constantBuffer[i] = new ConstantBuffer(sizeof(Transform));
	}
}
