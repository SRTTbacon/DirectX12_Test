//#define BIAS 0.0006f        //シャギー抑制のバイアス値
#define SHADOWSIZE 8192.0f  //シャドウマップのサイズ

SamplerState texSampler : register(s0); //テクスチャ用サンプラー
SamplerComparisonState shadowSampler : register(s1); //影用サンプラー
Texture2D<float4> _MainTex : register(t0); //ベーステクスチャ
Texture2D<float4> _NormalMap : register(t1); //ノーマルマップ
Texture2D<float> shadowMap : register(t2); //シャドウマップ

//定数バッファ
cbuffer PixelBuffer : register(b0)
{
    float4 lightDir;        //ライトの方向
    float4 ambientColor;    //影の色
    float4 diffuseColor;    //標準の色
    float4 cameraEyePos;    //カメラの位置
    float4 fogColor;        //フォグの色
    float2 fogStartEnd;     //フォグの開始距離、終了距離
};

struct VSOutput
{
    float4 svpos : SV_POSITION; //座標
    float3 normal : NORMAL; //法線
    float2 uv : TEXCOORD0; //UV
    float4 shadowPos : TEXCOORD1; //影の位置
    float4 tanLightDir : TEXCOORD2; //従法線
    float3 tanHalfWayVec : TEXCOORD3; //接線
};

//法線と光の方向に応じたバイアス
float ComputeShadowBias(float3 normal, float3 lightDir)
{
    float cosTheta = saturate(dot(normal, lightDir));
    return max(0.0015 * (1.0 - cosTheta), 0.0005f); //法線が光と平行なほどバイアスを減らす
}

//Toon調の段階的な光
float ToonShading(float intensity)
{
    if (intensity > 0.5f)
        return 1.0f;
    else
        return 0.2f;
}

static const float2 poissonDisk[8] =
{
    float2(-0.326212, -0.405810),
    float2(-0.840144, -0.073580),
    float2(-0.695914, 0.457137),
    float2(-0.203345, 0.620716),
    float2(0.962340, -0.194983),
    float2(0.473434, -0.480026),
    float2(0.519456, 0.767022),
    float2(0.185461, -0.893124)
};

float ShadowCalculation(float4 shadowPos, float bias)
{
    float3 posFromLightVP = shadowPos.xyz / shadowPos.w;
    float2 shadowUV = (posFromLightVP.xy + float2(1.0f, -1.0f)) * float2(0.5f, -0.5f);

    float total = 0.0f;
    float2 texelSize = float2(1.0f / SHADOWSIZE, 1.0f / SHADOWSIZE);
    
    for (int i = 0; i < 8; ++i)
    {
        float2 sampleOffset = poissonDisk[i] * texelSize;
        float2 shadowSampleUV = shadowUV + sampleOffset;
        float sampleDepth = shadowMap.SampleCmpLevelZero(shadowSampler, shadowSampleUV, posFromLightVP.z - bias);
        total += sampleDepth;
    }

    return total / 8.0f; // 平均化
}

float4 main(VSOutput input) : SV_TARGET
{
    //テクスチャ
    float4 tex = _MainTex.Sample(texSampler, input.uv);

    //法線マップ
    float2 tiledUV = input.uv * float2(5.0f, 10.0f);
    float4 normalTexColor = _NormalMap.Sample(texSampler, tiledUV);
    
    //ライトの方向を逆に
    float3 lightVec = lightDir.xyz;
    float lightIntensity = saturate(dot(input.normal, -lightVec));
    //lightIntensity = smoothstep(0.49, 0.51, lightIntensity);
    lightIntensity = ToonShading(lightIntensity);
    
    //シャドウの計算
    float bias = ComputeShadowBias(input.normal, lightVec);
    float shadowFactor = ShadowCalculation(input.shadowPos, bias);
    shadowFactor = smoothstep(0.35, 0.85, shadowFactor);
    
    float shadowPower = lightIntensity * shadowFactor;
    shadowPower = max(0.5f, shadowPower);

    //シャドウがかかっていれば光を減少させる (0.0f なら完全な影、1.0f なら影なし)
    float4 lighting = lerp(ambientColor, diffuseColor, shadowPower);

    //ライティング結果とテクスチャカラーを掛け合わせる
    float4 finalColor = lighting * tex;
    
    return finalColor;
}
