#define BIAS 0.0001f     //シャギー抑制のバイアス値

SamplerState texSampler : register(s0); //テクスチャ用サンプラー
SamplerComparisonState shadowSampler : register(s1); //テクスチャ用サンプラー
Texture2D<float> shadowMap : register(t1); //シャドウマップ

//定数バッファ
cbuffer LightBuffer : register(b0)
{
    float3 lightDir;        //ライトの方向
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
    float3 posFromLightVP = shadowPos.xyz / shadowPos.w;
    float2 shadowUV = (posFromLightVP.xy + float2(1.0f, -1.0f)) * float2(0.5f, -0.5f);

    //影の柔らかさを決定するための分割数(サンプリング数)
    int numSamples = 4; //サンプル数 (多いほど滑らかだが重い)
    float total = 0.0f;
    float2 texelSize = float2(1.0f / 8192.0f, 1.0f / 8192.0f);
    
    //シャドウマップ上でのサンプリング
    for (int i = -numSamples / 2; i < numSamples / 2; ++i)
    {
        for (int j = -numSamples / 2; j < numSamples / 2; ++j)
        {
            //サンプルの位置
            float2 sampleOffset = float2(i, j) * texelSize;

            //サンプリング位置のUV座標
            float2 shadowSampleUV = shadowUV + sampleOffset;

            //シャドウマップから深度をサンプリング
            float sampleDepth = shadowMap.SampleCmpLevelZero(shadowSampler, shadowSampleUV, posFromLightVP.z - BIAS);
            
            //サンプルを合計
            total += sampleDepth;
        }
    }

    //平均を取る
    float shadowFactor = total / (numSamples * numSamples);

    //シャドウの有無を決定(0.0fなら完全な影、1.0fなら影なし)
    return shadowFactor;
}

float4 pixel(VSOutput input) : SV_TARGET
{
    //ライトの方向を逆に
    float lightIntensity = saturate(dot(input.normal, -lightDir));

    // シャドウの計算
    float shadowFactor = ShadowCalculation(input.shadowPos);
    
    float3 diffuse = float3(1.0f, 1.0f, 1.0f) * lightIntensity;

    //シャドウがかかっていれば光を減少させる (0.0f なら完全な影、1.0f なら影なし)
    float3 lighting = ambientColor + shadowFactor * diffuse;

    //ライティング結果とテクスチャカラーを掛け合わせる
    float4 finalColor = float4(lighting, 1.0f);

    return finalColor;
}