#pragma once
#include "..\\Engine.h"

#include "..\\Core\\SharedStruct\\SharedStruct.h"
#include "..\\Core\\VertexBuffer\\VertexBuffer.h"
#include "..\\Core\\RootSignature\\RootSignature.h"
#include "..\\Core\\PipelineState\\PipelineState.h"
#include "..\\\Core\\IndexBuffer\\IndexBuffer.h"
#include "..\\Core\\FBXLoader\\FBXLoader.h"
#include "..\\Core\\DescriptorHeap\\DescriptorHeap.h"
#include "..\\Core\\Texture2D\\Texture2D.h"

#include "..\\Camera\\Camera.h"

class Model
{
public:
	void Initialize(const wchar_t* fileName, Camera* camera, bool bCharacter);

	void Update();
	void Draw();

	void SetPosition(DirectX::XMFLOAT3 pos);

	Model();

private:
	DescriptorHeap* descriptorHeap;										//GPUに送るリソースを増やす感じ
	std::vector<DescriptorHandle*> materialHandles;						//テクスチャ用のハンドル一覧(FBX内のマテリアル数と同じ)

	VertexBuffer* vertexBuffer;											//頂点バッファ(頂点の配列。GPUに頂点データを送るため毎フレームメモリからコピーするから重い。改良したい)
	RootSignature* rootSignature;										//ルートシグネチャ(恐らくGPUに送るデータを管理するやつ)
	PipelineState* pipelineState;										//パイプライン(描画方法。シェーダーの設定など)
	IndexBuffer* indexBuffer;											//インデックスバッファ(頂点に番号を付けるやつ)

	Camera* m_camera;													//カメラのポインタ
	ConstantBuffer* m_constantBuffer[Engine::FRAME_BUFFER_COUNT];		//コンスタントバッファ(カメラからどう見えるか)。モデルのちらつき防止のため2つ用意する

	DirectX::XMFLOAT3 m_pos;
	DirectX::XMFLOAT3 m_rot;

	std::vector<VertexBuffer*> vertexBuffers;							//メッシュの数分の頂点バッファ
	std::vector<IndexBuffer*> indexBuffers;								//メッシュの数分のインデックスバッファ

	std::vector<Mesh> meshes;											//メッシュ

	FBXLoader loader;

	float m_animTime;
};