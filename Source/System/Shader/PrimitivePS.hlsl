#define BIAS 0.0001f     //シャギー抑制のバイアス値
#define SHADOWSIZE 8192.0f

SamplerState texSampler : register(s0);         //テクスチャ用サンプラー
SamplerComparisonState shadowSampler : register(s1); //テクスチャ用サンプラー
Texture2D<float4> _MainTex : register(t0);      //テクスチャ
Texture2D<float4> _NormalMap : register(t1);    //ノーマルマップ
Texture2D<float> shadowMap : register(t2);      //シャドウマップ

//定数バッファ
cbuffer LightBuffer : register(b0)
{
    float4 lightDir;        //ライトの方向
    float4 ambientColor;    //影色
    float4 diffuseColor;    //標準の色
    float4 cameraEyePos;    //カメラの位置
};

struct VSOutput
{
    float4 svpos : SV_POSITION;     //頂点シェーダーから来た座標
    float3 normal : NORMAL;         //頂点シェーダーから北ノーマルマップ
    float2 uv : TEXCOORD;           //頂点シェーダーから来たUV
    float4 shadowPos : TEXCOORD1;   //影の位置
};

float IsUVOutOfRange(float2 uv)
{
    float uBelowZero = 1.0 - step(0.0, uv.x);
    float uAboveOne = step(1.0, uv.x);

    float vBelowZero = 1.0 - step(0.0, uv.y);
    float vAboveOne = step(1.0, uv.y);

    float isOutOfRange = uBelowZero + uAboveOne + vBelowZero + vAboveOne;
    return saturate(isOutOfRange);
}

float ShadowCalculation(float4 worldPos, float4 shadowPos)
{
    float3 posFromLightVP = shadowPos.xyz / shadowPos.w;
    float2 shadowUV = (posFromLightVP.xy + float2(1.0f, -1.0f)) * float2(0.5f, -0.5f);
    
    //影の柔らかさを決定するための分割数(サンプリング数)
    int numSamples = 4; //サンプル数 (多いほど滑らかだが重い)
    float total = 0.0f;
    float2 texelSize = float2(1.0f / SHADOWSIZE, 1.0f / SHADOWSIZE);
    
    // 距離に応じたバイアス
    float dynamicBias = lerp(BIAS, 0.001f, saturate(length(worldPos - cameraEyePos) / 50.0f));

    //シャドウマップ上でのサンプリング
    for (int i = -numSamples / 2; i < numSamples / 2; ++i)
    {
        for (int j = -numSamples / 2; j < numSamples / 2; ++j)
        {
            //サンプルの位置
            float2 sampleOffset = float2(i, j) * texelSize;

            //サンプリング位置のUV座標
            float2 shadowSampleUV = shadowUV + sampleOffset;
            //shadowSampleUV = saturate(shadowSampleUV);

            //シャドウマップから深度をサンプリング
            float sampleDepth = shadowMap.SampleCmpLevelZero(shadowSampler, shadowSampleUV, posFromLightVP.z - dynamicBias);
            
            //サンプルを合計
            total += sampleDepth;
        }
    }

    
    //平均を取る
    float shadowFactor = total / (numSamples * numSamples);
    //shadowFactor = lerp(shadowFactor, 1.0f, IsUVOutOfRange(shadowUV));

    //シャドウの有無を決定(0.0fなら完全な影、1.0fなら影なし)
    return shadowFactor;
}

float4 pixel(VSOutput input) : SV_TARGET
{
    //ライトの方向を逆に
    float lightIntensity = saturate(dot(input.normal, -lightDir.xyz));

    //シャドウの計算
    float shadowFactor = ShadowCalculation(input.svpos, input.shadowPos);
    
    float4 diffuse = diffuseColor * lightIntensity;

    //シャドウがかかっていれば光を減少させる (0.0f なら完全な影、1.0f なら影なし)
    float4 lighting = ambientColor + shadowFactor * diffuse;

    return lighting;
}