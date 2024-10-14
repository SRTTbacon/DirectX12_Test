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

	//スタティックサンプラーの設定
	auto sampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	//ルートシグニチャの設定（ルートパラメーターとスタティックサンプラーを入れる）
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = m_rootParamSize;		//ルートパラメーターの個数をいれる
	desc.NumStaticSamplers = 1;					//サンプラーの個数をいれる
	desc.pParameters = rootParam;				//ルートパラメーターのポインタをいれる
	desc.pStaticSamplers = &sampler;			//サンプラーのポインタを入れる
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
		m_rootParamSize = 6;
		CD3DX12_ROOT_PARAMETER* rootParam = new CD3DX12_ROOT_PARAMETER[m_rootParamSize];
		rootParam[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // b0の定数バッファを設定、全てのシェーダーから見えるようにする
		rootParam[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX); // b1の定数バッファを設定、全てのシェーダーから見えるようにする
		rootParam[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_VERTEX); // b2の定数バッファを設定、全てのシェーダーから見えるようにする

		//シェイプキー用をスロットt1に設定
		rootParam[3].InitAsShaderResourceView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		//シェイプキーのウェイト用をスロットt2に設定
		rootParam[4].InitAsShaderResourceView(2, 0, D3D12_SHADER_VISIBILITY_VERTEX);

		m_pTableRange1 = new CD3DX12_DESCRIPTOR_RANGE[1];				//ディスクリプタテーブル
		//テクスチャ用をスロットt0に設定
		m_pTableRange1[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);	//シェーダーリソースビュー
		rootParam[5].InitAsDescriptorTable(1, m_pTableRange1, D3D12_SHADER_VISIBILITY_PIXEL);

		return rootParam;
	}
	//単色のシェーダー
	else if (m_shaderKind == ShaderKinds::PrimitiveShader)
	{
		m_rootParamSize = 1;
		CD3DX12_ROOT_PARAMETER* rootParam = new CD3DX12_ROOT_PARAMETER[m_rootParamSize];
		rootParam[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // b0の定数バッファを設定、全てのシェーダーから見えるようにする
		
		return rootParam;
	}
	return nullptr;
}
