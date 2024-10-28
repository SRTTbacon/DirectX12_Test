#include "RootSignature.h"

RootSignature::RootSignature(ID3D12Device* device, ShaderKinds shaderKind)
	: m_shaderKind(shaderKind)
	, m_rootParamSize(0)
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
	CD3DX12_STATIC_SAMPLER_DESC samplers[2]{};
	samplers[0] = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
	samplers[1] = CD3DX12_STATIC_SAMPLER_DESC(1, D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 0.0f, 0, D3D12_COMPARISON_FUNC_LESS_EQUAL);
	samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	//ルートシグニチャの設定（ルートパラメーターとスタティックサンプラーを入れる）
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = m_rootParamSize;		//ルートパラメーターの個数をいれる
	desc.NumStaticSamplers = 2;					//サンプラーの個数をいれる
	desc.pParameters = rootParam;				//ルートパラメーターのポインタをいれる
	desc.pStaticSamplers = samplers;			//サンプラーのポインタを入れる
	desc.Flags = flag;							//フラグを設定

	ComPtr<ID3DBlob> pBlob;
	ComPtr<ID3DBlob> pErrorBlob;

	//シリアライズ
	auto hr = D3D12SerializeRootSignature(
		&desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		pBlob.GetAddressOf(),
		pErrorBlob.GetAddressOf());
	if (FAILED(hr))
	{
		printf("ルートシグネチャシリアライズに失敗\n");
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
		printf("ルートシグネチャの生成に失敗\n");
		return;
	}

	if (m_pTableRange1)
		delete[] m_pTableRange1;
	delete[] rootParam;
}

ID3D12RootSignature* RootSignature::GetRootSignature() const {
    return rootSignature.Get();
}

CD3DX12_ROOT_PARAMETER* RootSignature::GetRootParameter()
{
	//ボーンが存在するシェーダーの場合
	if (m_shaderKind == ShaderKinds::BoneShader) {
		m_rootParamSize = 7;
		CD3DX12_ROOT_PARAMETER* rootParam = new CD3DX12_ROOT_PARAMETER[m_rootParamSize];
		rootParam[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);	//頂点シェーダーのb0の定数バッファを設定

		rootParam[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);		//ピクセルシェーダーのb0の定数バッファを設定

		m_pTableRange1 = new CD3DX12_DESCRIPTOR_RANGE[1];				//ディスクリプタテーブル
		//テクスチャ用をスロットt0に設定
		m_pTableRange1[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);	//シェーダーリソースビュー
		rootParam[2].InitAsDescriptorTable(1, m_pTableRange1, D3D12_SHADER_VISIBILITY_PIXEL);

		rootParam[3].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);	//頂点シェーダーのb1の定数バッファを設定
		rootParam[4].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_VERTEX);	//頂点シェーダーのb2の定数バッファを設定

		//シェイプキー用をスロットt1に設定
		rootParam[5].InitAsShaderResourceView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		//シェイプキーのウェイト用をスロットt2に設定
		rootParam[6].InitAsShaderResourceView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);

		return rootParam;
	}
	//単色のシェーダー
	else if (m_shaderKind == ShaderKinds::PrimitiveShader) {
		m_rootParamSize = 3;
		CD3DX12_ROOT_PARAMETER* rootParam = new CD3DX12_ROOT_PARAMETER[m_rootParamSize];
		rootParam[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);	//頂点シェーダーのb0の定数バッファを設定
		rootParam[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);		//ピクセルシェーダーのb0の定数バッファを設定

		m_pTableRange1 = new CD3DX12_DESCRIPTOR_RANGE[1];				//ディスクリプタテーブル
		//テクスチャ用をスロットt0に設定
		m_pTableRange1[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);	//シェーダーリソースビュー
		rootParam[2].InitAsDescriptorTable(1, m_pTableRange1, D3D12_SHADER_VISIBILITY_PIXEL);
		
		return rootParam;
	}
	//影のみのシェーダー
	else if (m_shaderKind == ShaderKinds::ShadowShader) {
		m_rootParamSize = 1;
		CD3DX12_ROOT_PARAMETER* rootParam = new CD3DX12_ROOT_PARAMETER[m_rootParamSize];
		//定数バッファをルートパラメータとして設定 (メッシュごとのワールド変換行列用)
		rootParam[0].InitAsConstantBufferView(0);

		return rootParam;
	}
	return nullptr;
}
