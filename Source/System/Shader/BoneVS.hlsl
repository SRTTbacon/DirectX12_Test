cbuffer ModelConstantBuffer : register(b0)
{
	matrix modelMatrix;       //モデルマトリックス
	matrix viewMatrix;        //ビューマトリックス
	matrix projectionMatrix;  //プロジェクションマトリックス
}

cbuffer BoneMatrices : register(b1)
{
	matrix boneMatrices[512];	//ボーンマトリックス（最大512個)
}

cbuffer ShapeWeights : register(b2)
{
    float shapeWeight[512];		//各シェイプキーのウェイト (最大512個)
}

struct VSInput
{
	float3 pos : POSITION;				//頂点座標
	float3 normal : NORMAL;				//法線
	float2 uv : TEXCOORD;				//UV
	float4 color : COLOR;				//頂点色
	float4 boneWeights : BONEWEIGHTS;	//各頂点のボーンの影響度
	uint4 boneIDs : BONEIDS;			//各頂点に影響を与えるボーンのインデックス
    float3 shapePos : SHAPEPOSITION;	//シェイプキーの適応後の位置
    uint shapeID : SHAPEID;				//対応するシェイプキーのインデックス
};

struct VSOutput
{
	float4 svpos : SV_POSITION; //座標
	float4 color : COLOR;		//色
	float2 uv : TEXCOORD;		//UV
};

VSOutput vert(VSInput input)
{
	VSOutput output = (VSOutput)0;
	
	//ボーンを反映
	matrix skinMatrix = input.boneWeights.x * boneMatrices[input.boneIDs.x] + input.boneWeights.y * boneMatrices[input.boneIDs.y] +
		input.boneWeights.z * boneMatrices[input.boneIDs.z] + input.boneWeights.w * boneMatrices[input.boneIDs.w];

    float3 shapePos = input.pos + input.shapePos * shapeWeight[input.shapeID];
	
    float4 localPos = mul(float4(shapePos, 1.0f), skinMatrix); //頂点座標
	//float4 localPos = float4(input.pos, 1.0f);				//頂点座標
	float4 worldPos = mul(modelMatrix, localPos);				//ワールド座標に変換
	float4 viewPos = mul(viewMatrix, worldPos);					//ビュー座標に変換
	float4 projPos = mul(projectionMatrix, viewPos);			//投影変換

	output.svpos = projPos;		//投影変換された座標
	output.color = input.color; //頂点色
	output.uv = input.uv;		//UV
	return output;				//ピクセルシェーダーに渡す
}
