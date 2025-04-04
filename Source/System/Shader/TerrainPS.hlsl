#define BIAS 0.001f     //シャギー抑制のバイアス値
#define SHADOWSIZE 8192.0f

//雪が積もるようなシェーダー
//メインテクスチャ + モデルの上方向にサブテクスチャを貼る

SamplerState texSampler : register(s0); //テクスチャ用サンプラー
SamplerComparisonState shadowSampler : register(s1); //シャドウマップ用サンプラー
Texture2D<float4> _MainTex : register(t0);  //メインテクスチャ
Texture2D<float4> _SubTex : register(t1);   //サブテクスチャ
Texture2D<float> shadowMap : register(t2);  //シャドウマップ

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

struct VSOutput
{
    float4 svpos : SV_POSITION; //来た座標
    float3 normal : NORMAL; //ノーマルマップ
    float2 uv : TEXCOORD; //来たUV
    float4 shadowPos : TEXCOORD1; //影の位置
    float3 worldPos : TEXCOORD2;    //ワールド座標
};

static const float2 poissonDisk[4] =
{
    float2(-0.94201624, -0.39906216),
    float2(0.94558609, -0.76890725),
    float2(-0.09418410, -0.92938870),
    float2(0.34495938, 0.29387760)
};

//法線と光の方向に応じたバイアス
float ComputeShadowBias(float3 normal, float3 lightDir)
{
    float cosTheta = saturate(dot(normal, lightDir));
    return max(0.0015 * (1.0 - cosTheta), 0.0005f); //法線が光と平行なほどバイアスを減らす
}

float IsUVOutOfRange(float2 uv)
{
    float uBelowZero = 1.0 - step(0.0, uv.x);
    float uAboveOne = step(1.0, uv.x);

    float vBelowZero = 1.0 - step(0.0, uv.y);
    float vAboveOne = step(1.0, uv.y);

    float isOutOfRange = uBelowZero + uAboveOne + vBelowZero + vAboveOne;
    return saturate(isOutOfRange);
}

float ShadowCalculation(float4 shadowPos, float bias)
{
    float3 posFromLightVP = shadowPos.xyz / shadowPos.w;
    float2 shadowUV = (posFromLightVP.xy + float2(1, -1)) * float2(0.5, -0.5);
    
    if (shadowUV.x <= 0.0f || shadowUV.x >= 1.0f || shadowUV.y <= 0.0f || shadowUV.y >= 1.0f)
    {
        return 1.0f;
    }
    
    //影の柔らかさを決定するための分割数(サンプリング数)
    float total = 0.0f;
    float2 texelSize = float2(1.0f / SHADOWSIZE, 1.0f / SHADOWSIZE);
    
    for (int i = 0; i < 4; ++i)
    {
        float2 sampleOffset = poissonDisk[i] * texelSize;
        float2 shadowSampleUV = shadowUV + sampleOffset;
        float sampleDepth = shadowMap.SampleCmpLevelZero(shadowSampler, shadowSampleUV, posFromLightVP.z - bias);
        total += sampleDepth;
    }
    
    //平均を取る
    //float shadowFactor = total / (numSamples * numSamples);
    float shadowFactor = total / 4.0f;

    //シャドウの有無を決定(0.0fなら完全な影、1.0fなら影なし)
    return shadowFactor;
}

float4 pixel(VSOutput input) : SV_TARGET
{
    //法線のY成分を利用してサブテクスチャの影響を決定
    float subTexAmount = saturate(input.normal.y * 1.0f);

    float2 subTexUV = input.uv * 5.0f; // XZ 平面でタイル状に

    //テクスチャ
    float4 mainTex = _MainTex.Sample(texSampler, input.uv);
    float4 subTex = _SubTex.Sample(texSampler, subTexUV);

    //ライトの方向を逆に
    float3 lightVec = lightDir.xyz;
    float lightIntensity = saturate(dot(input.normal, -lightVec));

    //シャドウの計算
    float bias = ComputeShadowBias(input.normal, lightVec);
    float shadowFactor = ShadowCalculation(input.shadowPos, bias);
    
    float shadowPower = lightIntensity * shadowFactor;
    shadowPower = max(0.1f, shadowPower);

    //シャドウがかかっていれば光を減少させる (0.0f なら完全な影、1.0f なら影なし)
    float4 diffuse = diffuseColor * shadowPower;

    float4 lighting = ambientColor + diffuse;
    
    lighting *= lerp(mainTex, subTex, subTexAmount);
    
    //フォグの処理
    //カメラからの距離計算
    float distance = abs(length(input.worldPos - cameraEyePos.xyz));

    //0 ～ 1にマッピング
    float fogFactor = saturate((distance - fogStartEnd.x) / (fogStartEnd.y - fogStartEnd.x));
    fogFactor = min(0.9f, fogFactor);
    
    lighting = lerp(lighting, fogColor, fogFactor);
    
    lighting.a = cameraEyePos.w;

    return lighting;
}