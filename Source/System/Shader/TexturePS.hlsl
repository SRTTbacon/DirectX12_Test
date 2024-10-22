#define BIAS 0.005f     //シャギー抑制のバイアス値

struct VSOutput
{
	float4 svpos : SV_POSITION;         //頂点シェーダーから来た座標
    float3 normal : NORMAL;             //頂点シェーダーから北ノーマルマップ
	float4 color : COLOR;		        //頂点シェーダーから来た色
	float2 uv : TEXCOORD;		        //頂点シェーダーから来たUV
    float4 lightSpacePos : TEXCOORD0;   //ディレクショナルライトの情報
    float3 lightDir : TEXCOORD1;        //ディレクショナルライトの方向
};

SamplerState texSampler : register(s0);	    //テクスチャ用サンプラー
Texture2D _MainTex : register(t0);	        //テクスチャ

SamplerState shadowSampler : register(s1);  //シャドウマップ用サンプラー
Texture2D shadowMap : register(t1);         //シャドウマップ

float ShadowCalculation(float4 lightSpacePos)
{
    //シャドウマップのテクスチャ座標に変換
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5f + 0.5f;

    //シャドウマップで深度をサンプル
    float shadowMapDepth = shadowMap.Sample(shadowSampler, projCoords.xy).r;
    
    //現在のピクセルの深度とシャドウマップの深度を比較
    float currentDepth = projCoords.z;
    return currentDepth > shadowMapDepth + BIAS ? 0.0f : 1.0f; // 影かどうかを判定
}

float4 pixel(VSOutput input) : SV_TARGET
{
    float4 tex = _MainTex.Sample(texSampler, input.uv);

    // シャドウ判定
    float shadow = ShadowCalculation(input.lightSpacePos);
    
    // ライトの影響を計算
    float3 light = max(dot(normalize(input.normal), -input.lightDir), 0.0f);

    return tex * float4(light, 1.0f) * shadow;
}
