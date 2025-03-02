#include "RootSignature.h"

RootSignature::RootSignature(ID3D12Device* device, ShaderKinds shaderKind)
	: m_shaderKind(shaderKind)
	, m_rootParamSize(0)
	, m_pDiscriptorRange(nullptr)
{
	auto flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;	//アプリケーションの入力アセンブラを使用する
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;			//ドメインシェーダーのルートシグネチャへのアクセスを拒否する
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;				//ハルシェーダーのルートシグネチャへのアクセスを拒否する
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;			//ジオメトリシェーダーのルートシグネチャへのアクセスを拒否する

	CD3DX12_ROOT_PARAMETER* rootParam = GetRootParameter();
	if (!rootParam) {
		printf("ShaderKindが正しく設定されていません。\n");
		return;
	}

	//スタティックサンプラーの設定
	int samplerCount = 0;
	CD3DX12_STATIC_SAMPLER_DESC* samplers = nullptr;

	//影用のシェーダー以外
	if (shaderKind != ShaderKinds::ShadowShader) {
		samplerCount = 2;
		samplers = new CD3DX12_STATIC_SAMPLER_DESC[samplerCount];
		samplers[0] = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
		samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		samplers[1] = CD3DX12_STATIC_SAMPLER_DESC(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		samplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	}

	//ルートシグニチャの設定（ルートパラメーターとスタティックサンプラーを入れる）
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = m_rootParamSize;		//ルートパラメーターの個数をいれる
	desc.NumStaticSamplers = samplerCount;		//サンプラーの個数をいれる
	desc.pParameters = rootParam;				//ルートパラメーターのポインタをいれる
	desc.pStaticSamplers = samplers;			//サンプラーのポインタを入れる
	desc.Flags = flag;							//フラグを設定

	ComPtr<ID3DBlob> pBlob;
	ComPtr<ID3DBlob> pErrorBlob;

	//シリアライズ
	HRESULT hr = D3D12SerializeRootSignature(
		&desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		pBlob.GetAddressOf(),
		pErrorBlob.GetAddressOf());
	if (FAILED(hr))
	{
		if (pErrorBlob) {
			printf("ルートシグネチャシリアライズに失敗 - %s\n", (char*)pErrorBlob->GetBufferPointer());
		}
		else {
			printf("ルートシグネチャシリアライズに失敗\n");
		}
		return;
	}

	//ルートシグネチャ生成
	hr = device->CreateRootSignature(
		0,												//GPUが複数ある場合のノードマスク（基本1つのため0を指定）
		pBlob->GetBufferPointer(),						//シリアライズしたデータのポインタ
		pBlob->GetBufferSize(),							//シリアライズしたデータのサイズ
		IID_PPV_ARGS(rootSignature.GetAddressOf()));	//ルートシグニチャ格納先のポインタ
	if (FAILED(hr))
	{
		HRESULT hr2 = device->GetDeviceRemovedReason();
		printf("ルートシグネチャの生成に失敗。 %u, %u, シェーダー : %d\n", (UINT)hr, (UINT)hr2, shaderKind);
		return;
	}

	if (m_pDiscriptorRange) {
		delete m_pDiscriptorRange;
		m_pDiscriptorRange = nullptr;
	}

	delete[] rootParam;
}

ID3D12RootSignature* RootSignature::GetRootSignature() const {
    return rootSignature.Get();
}

CD3DX12_ROOT_PARAMETER* RootSignature::GetRootParameter()
{
	//ボーンが存在するシェーダーの場合
	if (m_shaderKind == ShaderKinds::BoneShader) {
		m_rootParamSize = 8;
		CD3DX12_ROOT_PARAMETER* rootParam = new CD3DX12_ROOT_PARAMETER[m_rootParamSize];
		rootParam[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);	//頂点シェーダーのb0の定数バッファを設定
		rootParam[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);		//ピクセルシェーダーのb0の定数バッファを設定

		//テクスチャ用をスロットt0に設定
		m_pDiscriptorRange = new CD3DX12_DESCRIPTOR_RANGE[3];
		//シャドウマップ用をスロットt2に設定 (どのマテリアルでも共通のため独立)
		m_pDiscriptorRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);	//シェーダーリソースビュー(1個)
		rootParam[2].InitAsDescriptorTable(1, &m_pDiscriptorRange[0], D3D12_SHADER_VISIBILITY_PIXEL);	//t2

		m_pDiscriptorRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);	//シェーダーリソースビュー(2個)
		rootParam[3].InitAsDescriptorTable(1, &m_pDiscriptorRange[1], D3D12_SHADER_VISIBILITY_PIXEL);	//t0、t1
		//シェイプキー
		m_pDiscriptorRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);	//t0
		rootParam[4].InitAsDescriptorTable(1, &m_pDiscriptorRange[2], D3D12_SHADER_VISIBILITY_VERTEX);

		rootParam[5].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);	//頂点シェーダーのb1の定数バッファを設定
		rootParam[6].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_VERTEX);	//頂点シェーダーのb2の定数バッファを設定

		//シェイプキーのウェイト用をスロットt1に設定
		rootParam[7].InitAsShaderResourceView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);

		return rootParam;
	}
	//単色のシェーダー
	else if (m_shaderKind == ShaderKinds::PrimitiveShader) {
		m_rootParamSize = 4;
		CD3DX12_ROOT_PARAMETER* rootParam = new CD3DX12_ROOT_PARAMETER[m_rootParamSize];
		rootParam[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);	//頂点シェーダーのb0の定数バッファを設定
		rootParam[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);		//ピクセルシェーダーのb0の定数バッファを設定

		//テクスチャ用をスロットt0に設定
		m_pDiscriptorRange = new CD3DX12_DESCRIPTOR_RANGE[2];
		m_pDiscriptorRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);	//シェーダーリソースビュー(1個)
		rootParam[2].InitAsDescriptorTable(1, &m_pDiscriptorRange[0], D3D12_SHADER_VISIBILITY_PIXEL);	//t2

		m_pDiscriptorRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);	//シェーダーリソースビュー(2個)
		rootParam[3].InitAsDescriptorTable(1, &m_pDiscriptorRange[1], D3D12_SHADER_VISIBILITY_PIXEL);	//t0、t1

		return rootParam;
	}
	//影のみのシェーダー
	else if (m_shaderKind == ShaderKinds::ShadowShader) {
		m_rootParamSize = 5;
		CD3DX12_ROOT_PARAMETER* rootParam = new CD3DX12_ROOT_PARAMETER[m_rootParamSize];
		//定数バッファをルートパラメータとして設定 (メッシュごとのワールド変換行列用)
		rootParam[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);	//頂点シェーダーのb0の定数バッファを設定
		rootParam[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);	//頂点シェーダーのb1の定数バッファを設定
		rootParam[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_VERTEX);	//頂点シェーダーのb2の定数バッファを設定

		//シェイプキー
		m_pDiscriptorRange = new CD3DX12_DESCRIPTOR_RANGE();
		m_pDiscriptorRange->Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);	//頂点シェーダー(1個)
		rootParam[3].InitAsDescriptorTable(1, m_pDiscriptorRange, D3D12_SHADER_VISIBILITY_VERTEX);	//t0

		//シェイプキーのウェイト用をスロットt1に設定
		rootParam[4].InitAsShaderResourceView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);

		return rootParam;
	}
	return nullptr;
}
