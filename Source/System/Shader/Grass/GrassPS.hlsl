#define SHADOWSIZE 8192.0f

//サンプラーとテクスチャ
SamplerState texSampler : register(s0); //テクスチャ用サンプラー
SamplerComparisonState shadowSampler : register(s1); //テクスチャ用サンプラー
Texture2D<float4> _MainTex : register(t0); //テクスチャ
Texture2D<float> _NoiseTex : register(t1); //テクスチャ
Texture2D<float> shadowMap : register(t2); //シャドウマップ

//定数バッファ
cbuffer PixelBuffer : register(b0)
{
    float4 lightDir;        //ライトの方向
    float4 ambientColor;    //影色
    float4 diffuseColor;    //標準の色
    float4 cameraEyePos;    //カメラの位置
    float4 fogColor;        //フォグの色
    float2 fogStartEnd;     //フォグの開始距離、終了距離
};

struct VS_OUTPUT
{
    float4 svpos : SV_POSITION; //スクリーン座標
    float3 normal : NORMAL; //法線
    float2 uv : TEXCOORD; //UV
    float4 shadowPos : TEXCOORD1; //影の位置
    float3 worldPos : TEXCOORD2; //ワールド座標
};

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

float IsUVOutOfRange(float2 uv)
{
    float uBelowZero = 1.0 - step(0.0, uv.x);
    float uAboveOne = step(1.0, uv.x);

    float vBelowZero = 1.0 - step(0.0, uv.y);
    float vAboveOne = step(1.0, uv.y);

    float isOutOfRange = uBelowZero + uAboveOne + vBelowZero + vAboveOne;
    return saturate(isOutOfRange);
}

//法線と光の方向に応じたバイアス
float ComputeShadowBias(float3 normal, float3 lightDir)
{
    float cosTheta = saturate(dot(normal, lightDir));
    return max(0.0015 * (1.0 - cosTheta), 0.0005f); //法線が光と平行なほどバイアスを減らす
}

float ShadowCalculation(float4 shadowPos, float bias)
{
    float3 posFromLightVP = shadowPos.xyz / shadowPos.w;
    float2 shadowUV = (posFromLightVP.xy + float2(1, -1)) * float2(0.5, -0.5);
    
    float isInside = step(0.0f, shadowUV.x) * step(shadowUV.x, 1.0f) *
                     step(0.0f, shadowUV.y) * step(shadowUV.y, 1.0f);

    float total = 0.0f;
    float2 texelSize = float2(1.0f / SHADOWSIZE, 1.0f / SHADOWSIZE);
    for (int i = 0; i < 8; ++i)
    {
        float2 sampleOffset = poissonDisk[i] * texelSize * 1.3f;
        float2 shadowSampleUV = shadowUV + sampleOffset;
        float sampleDepth = shadowMap.SampleCmpLevelZero(shadowSampler, shadowSampleUV, posFromLightVP.z - bias);
        total += sampleDepth;
    }

    //平均を取る
    float shadowFactor = total / 8.0f;
    
    shadowFactor = lerp(1.0f, shadowFactor, isInside);

    //シャドウの有無を決定(0.0fなら完全な影、1.0fなら影なし)
    return shadowFactor;
}

// ピクセルシェーダー
float4 main(VS_OUTPUT input) : SV_TARGET
{
    // 草のテクスチャから色とアルファをサンプリング
    float4 grassTex = _MainTex.Sample(texSampler, input.uv);
    
    // アルファテスト：アルファが小さい部分は描画しない
    if (grassTex.a <= 0.29f) //透明部分を無視
    {
        discard;
    }

    // ライティング計算（単純なディフューズライティング）
    float3 lightVec = lightDir.xyz;
    float lightIntensity = saturate(dot(input.normal, -lightVec));

    float bias = ComputeShadowBias(input.normal, lightVec);

    //シャドウの計算
    float shadowFactor = ShadowCalculation(input.shadowPos, bias);
    
    float shadowPower = 1.0f * shadowFactor;
    shadowPower = max(0.1f, shadowPower);

    //シャドウがかかっていれば光を減少させる (0.0f なら完全な影、1.0f なら影なし)
    float4 diffuse = lerp(ambientColor, diffuseColor, shadowPower);

    float2 noiseUV = float2(input.worldPos.x, input.worldPos.z) * -0.02f;
    float noiseTex = _NoiseTex.Sample(texSampler, noiseUV);
    float4 grassColor1 = float4(0.32f, 0.67f, 0.24f, 1.0f);
    float4 grassColor2 = float4(0.7f, 1.0f, 0.19f, 1.0f);
    float4 finalColor = lerp(grassColor1, grassColor2, (1.0f * noiseTex));

    // ライティングの影響を草の色に加算
    finalColor *= diffuse;

    return finalColor;
}
