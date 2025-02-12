cbuffer ModelConstantBuffer : register(b0)
{
	matrix modelMatrix;			//モデルマトリックス
	matrix viewMatrix;			//ビューマトリックス
	matrix projectionMatrix;	//プロジェクションマトリックス
    matrix lightViewProjMatrix;	//ディレクショナルライトの情報
    matrix normalMatrix;		//モデルのスケール、回転などをinput.normalにも適応する用
    float4 eyePos;              //カメラの位置
    float4 lightPos;            //ライトの位置
}

cbuffer BoneMatrices : register(b1)
{
	matrix boneMatrices[512];	    //ボーンマトリックス（最大512個)
}

cbuffer Constants : register(b2)	//初期化時に1度だけしか実行しないもの
{
    uint vertexCount;				//頂点数
    uint shapeCount;				//シェイプキーの数
};

Texture2D<float4> ShapeDeltasTexture : register(t0);        //シェイプキーごとの位置変位データ
StructuredBuffer<float> ShapeWeights : register(t1);		//各シェイプキーのウェイト

float4x4 InvTangentMatrix(float3 tangent, float3 binormal, float3 normal);

struct VSInput
{
	float3 pos : POSITION;				//頂点座標
    float4 boneWeights : BONEWEIGHTS;	//各頂点のボーンの影響度
    uint4 boneIDs : BONEIDS;			//各頂点に影響を与えるボーンのインデックス
	uint vertexID : VERTEXID;			//頂点のID
    float3 normal : NORMAL;             //法線
	float2 uv : TEXCOORD;				//UV
    float3 tangent : TANGENT;           //接線
    float3 binormal : BINORMAL;         //従法線
};

struct VSOutput
{
	float4 svpos : SV_POSITION;         //座標
    float3 normal : NORMAL;		        //ノーマルマップ
	float2 uv : TEXCOORD0;		        //UV
    float4 shadowPos : TEXCOORD1;       //セルフシャドウの投影位置
    float4 tanLightDir : TEXCOORD2;     //接線
    float3 tanHalfWayVec : TEXCOORD3;   //従法線
};

//頂点IDとシェイプキーのIDから相対位置を取得
float3 GetShapeDelta(uint vertexID, uint shapeID)
{
    //ウェイトが非常に小さい場合は無視
    float weight = ShapeWeights[shapeID];
    if (weight < 0.001f)
        return float3(0.0f, 0.0f, 0.0f);
    
    //シェイプキーのテクスチャサイズを取得
    uint width = 0, height = 0;
    ShapeDeltasTexture.GetDimensions(width, height);

    //座標
    uint x = vertexID % width;
    uint y = (vertexID / width) * shapeCount + shapeID;

    return ShapeDeltasTexture.Load(int3(x, y, 0)).xyz * weight;
}

VSOutput vert(VSInput input)
{
	VSOutput output = (VSOutput)0;
	
	//ボーンを反映
    matrix skinMatrix = input.boneWeights.x * boneMatrices[input.boneIDs.x] + input.boneWeights.y * boneMatrices[input.boneIDs.y] +
		input.boneWeights.z * boneMatrices[input.boneIDs.z] + input.boneWeights.w * boneMatrices[input.boneIDs.w];

	//シェイプキーの影響を加算
    float3 shapePos = input.pos;
    for (uint i = 0; i < shapeCount; i++)
    {
        shapePos += GetShapeDelta(input.vertexID, i);
    }
	
    float4 localPos = mul(float4(shapePos, 1.0f), skinMatrix);	//頂点座標
	float4 worldPos = mul(modelMatrix, localPos);				//ワールド座標に変換
	float4 viewPos = mul(viewMatrix, worldPos);					//ビュー座標に変換
	float4 projPos = mul(projectionMatrix, viewPos);			//投影変換

	//ボーンアニメーションによる法線変換
    float3 skinnedNormal = float3(0.0f, 0.0f, 0.0f);
    skinnedNormal += input.boneWeights.x * mul(input.normal, (float3x3)boneMatrices[input.boneIDs.x]);
    skinnedNormal += input.boneWeights.y * mul(input.normal, (float3x3)boneMatrices[input.boneIDs.y]);
    skinnedNormal += input.boneWeights.z * mul(input.normal, (float3x3)boneMatrices[input.boneIDs.z]);
    skinnedNormal += input.boneWeights.w * mul(input.normal, (float3x3)boneMatrices[input.boneIDs.w]);
    
    float3 nor = normalize(mul(input.normal, (float3x3)modelMatrix));
    float3 bi = normalize(mul(input.binormal, (float3x3)modelMatrix));
    float3 tan = normalize(mul(input.tangent, (float3x3)modelMatrix));
    //float3 nor = normalize(input.normal);
    //float3 bi = normalize(input.binormal);
    //float3 tan = normalize(input.tangent);

    float4x4 invMat = InvTangentMatrix(tan, bi, nor);

    //output.tanLightDir = mul(lightViewProjMatrix._41_42_43_44, invMat).xyz;
    output.tanLightDir = mul(lightPos, invMat);
    
    float4 halfWayVec = normalize(normalize(eyePos - projPos) + lightPos);
    output.tanHalfWayVec = mul(halfWayVec, invMat).xyz;
	
	output.svpos = projPos;			//投影変換された座標
    output.normal = normalize(mul(normalMatrix, float4(skinnedNormal, 1.0f)).xyz);
	output.uv = input.uv;			//UV
    output.shadowPos = mul(lightViewProjMatrix, worldPos);

	return output;				//ピクセルシェーダーに渡す
}


float4x4 InvTangentMatrix(float3 tangent, float3 binormal, float3 normal)
{
    float4x4 mat =
    {
        float4(tangent, 0.0f),
        float4(binormal, 0.0f),
        float4(normal, 0.0f),
        { 0.0f, 0.0f, 0.0f, 1.0f }
    };
    return transpose(mat); //転置
}