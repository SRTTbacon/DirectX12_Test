cbuffer ModelConstantBuffer : register(b0)
{
	matrix modelMatrix;			//モデルマトリックス
	matrix viewMatrix;			//ビューマトリックス
	matrix projectionMatrix;	//プロジェクションマトリックス
    matrix lightViewProjMatrix;	//ディレクショナルライトの情報
    matrix normalMatrix;		//モデルのスケール、回転などをinput.normalにも適応する用
}

cbuffer BoneMatrices : register(b1)
{
	matrix boneMatrices[512];	//ボーンマトリックス（最大512個)
}

cbuffer Constants : register(b2)	//初期化時に1度だけしか実行しないもの
{
    uint vertexCount;				//頂点数
    uint shapeCount;				//シェイプキーの数
};

StructuredBuffer<float3> ShapeDeltasTexture : register(t0);		//シェイプキーごとの位置変位データ
StructuredBuffer<float> ShapeWeights : register(t1);			//各シェイプキーのウェイト

struct VSInput
{
	float3 pos : POSITION;				//頂点座標
    float4 boneWeights : BONEWEIGHTS;	//各頂点のボーンの影響度
    uint4 boneIDs : BONEIDS;			//各頂点に影響を与えるボーンのインデックス
	float3 normal : NORMAL;				//法線
	float2 uv : TEXCOORD;				//UV
	uint vertexID : VERTEXID;			//頂点のID
};

struct VSOutput
{
	float4 svpos : SV_POSITION; //座標
    float3 normal : NORMAL;		//ノーマルマップ
	float2 uv : TEXCOORD;		//UV
    float4 shadowPos : TEXCOORD1;
};

//頂点IDとシェイプキーのIDから相対位置を取得
float3 GetShapeDelta(uint vertexID, uint shapeID)
{
    return ShapeDeltasTexture[vertexID + shapeID * vertexCount] * ShapeWeights[shapeID];
}

VSOutput vert(VSInput input)
{
	VSOutput output = (VSOutput)0;
	
	//ボーンを反映
    matrix skinMatrix = input.boneWeights.x * boneMatrices[input.boneIDs.x] + input.boneWeights.y * boneMatrices[input.boneIDs.y] +
		input.boneWeights.z * boneMatrices[input.boneIDs.z] + input.boneWeights.w * boneMatrices[input.boneIDs.w];

    float3 shapePos = input.pos;

	// シェイプキーの影響を加算
    for (uint i = 0; i < shapeCount; i++)
    {
        shapePos += GetShapeDelta(input.vertexID, i);
    }
	
    float4 localPos = mul(float4(shapePos, 1.0f), skinMatrix);	//頂点座標
	float4 worldPos = mul(modelMatrix, localPos);				//ワールド座標に変換
	float4 viewPos = mul(viewMatrix, worldPos);					//ビュー座標に変換
	float4 projPos = mul(projectionMatrix, viewPos);			//投影変換

	// ボーンアニメーションによる法線変換
    float3 skinnedNormal = float3(0.0f, 0.0f, 0.0f);
    skinnedNormal += input.boneWeights.x * mul(input.normal, (float3x3)boneMatrices[input.boneIDs.x]);
    skinnedNormal += input.boneWeights.y * mul(input.normal, (float3x3)boneMatrices[input.boneIDs.y]);
    skinnedNormal += input.boneWeights.z * mul(input.normal, (float3x3)boneMatrices[input.boneIDs.z]);
    skinnedNormal += input.boneWeights.w * mul(input.normal, (float3x3)boneMatrices[input.boneIDs.w]);
	
	output.svpos = projPos;			//投影変換された座標
    output.normal = normalize(mul(normalMatrix, float4(skinnedNormal, 1.0f)).xyz);
	output.uv = input.uv;			//UV
    output.shadowPos = mul(lightViewProjMatrix, worldPos);

	return output;				//ピクセルシェーダーに渡す
}
