cbuffer ModelConstantBuffer : register(b0) {
	matrix modelMatrix;       // モデルマトリックス
	matrix viewMatrix;        // ビューマトリックス
	matrix projectionMatrix;  // プロジェクションマトリックス
}

cbuffer BoneMatrices : register(b1) {
	matrix boneMatrices[512]; // ボーンマトリックス（最大512個のボーンをサポート）
}

struct VSInput
{
	float3 pos : POSITION; // 頂点座標
	float3 normal : NORMAL; // 法線
	float2 uv : TEXCOORD; // UV
	float4 color : COLOR; // 頂点色
	float4 boneWeights : BONEWEIGHTS;  // 各頂点のボーンウェイト
	uint4 boneIDs : BONEIDS;       // 各頂点に影響を与えるボーンID
};

struct VSOutput
{
	float4 svpos : SV_POSITION; // 変換された座標
	float4 color : COLOR; // 変換された色
	float2 uv : TEXCOORD;
};

VSOutput vert(VSInput input)
{
	VSOutput output = (VSOutput)0; // アウトプット構造体を定義する
	
	matrix skinMatrix = input.boneWeights.x * boneMatrices[input.boneIDs.x] + input.boneWeights.y * boneMatrices[input.boneIDs.y] +
		input.boneWeights.z * boneMatrices[input.boneIDs.z] + input.boneWeights.w * boneMatrices[input.boneIDs.w];

	float4 localPos = mul(float4(input.pos, 1.0f), skinMatrix); // 頂点座標
	//float4 localPos = float4(input.pos, 1.0f); // 頂点座標
	float4 worldPos = mul(modelMatrix, localPos); // ワールド座標に変換
	float4 viewPos = mul(viewMatrix, worldPos); // ビュー座標に変換
	float4 projPos = mul(projectionMatrix, viewPos); // 投影変換

	output.svpos = projPos; // 投影変換された座標をピクセルシェーダーに渡す
	output.color = input.color; // 頂点色をそのままピクセルシェーダーに渡す
	output.uv = input.uv;
	return output;
}
