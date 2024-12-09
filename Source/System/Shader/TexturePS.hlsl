#define BIAS 0.005f     //シャギー抑制のバイアス値

SamplerState texSampler : register(s0); //テクスチャ用サンプラー
SamplerComparisonState shadowSampler : register(s1); //テクスチャ用サンプラー
Texture2D<float4> _MainTex : register(t0); //テクスチャ
Texture2D<float> shadowMap : register(t1); //シャドウマップ

//定数バッファ
cbuffer LightBuffer : register(b0)
{
    float3 lightDir; //ライトの方向
    float3 ambientColor;
    float3 diffuseColor;
};

struct VSOutput
{
    float4 svpos : SV_POSITION; //頂点シェーダーから来た座標
    float3 normal : NORMAL; //頂点シェーダーから北ノーマルマップ
    float2 uv : TEXCOORD; //頂点シェーダーから来たUV
    float4 shadowPos : TEXCOORD1;
};

float ShadowCalculation(float4 shadowPos)
{
    float shadowFactor = 1.0f;
    
    // シャドウマップ座標変換
    float3 posFromLightVP = shadowPos.xyz / shadowPos.w;
    float2 shadowUV = (posFromLightVP.xy + float2(1.0f, -1.0f)) * float2(0.5f, -0.5f);

    //シャドウの有無を判定
    shadowFactor = shadowMap.SampleCmp(shadowSampler, shadowUV, posFromLightVP.z - BIAS);

    return shadowFactor;
}

float4 pixel(VSOutput input) : SV_TARGET
{
    //テクスチャ
    float4 tex = _MainTex.Sample(texSampler, input.uv);

    //法線を正規化
    float3 normal = normalize(input.normal);

    //ライトの方向を逆に
    float lightIntensity = saturate(dot(normal, -lightDir));

    //シャドウの計算
    float shadowFactor = ShadowCalculation(input.shadowPos);
    
    float3 diffuse = float3(1.0f, 1.0f, 1.0f) * lightIntensity;

    //シャドウがかかっていれば光を減少させる (0.0f なら完全な影、1.0f なら影なし)
    float3 lighting = ambientColor + shadowFactor * diffuse;

    //ライティング結果とテクスチャカラーを掛け合わせる
    float4 finalColor = float4(lighting, 1.0f) * tex;

    return finalColor;
}
