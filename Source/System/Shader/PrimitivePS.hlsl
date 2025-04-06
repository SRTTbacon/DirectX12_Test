#define BIAS 0.001f     //シャギー抑制のバイアス値
#define SHADOWSIZE 8192.0f

SamplerState texSampler : register(s0);         //テクスチャ用サンプラー
SamplerComparisonState shadowSampler : register(s1); //テクスチャ用サンプラー
Texture2D<float4> _MainTex : register(t0);      //テクスチャ
Texture2D<float4> _NormalMap : register(t1);    //ノーマルマップ
Texture2D<float> shadowMap : register(t2);      //シャドウマップ

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
    float4 svpos : SV_POSITION;     //来た座標
    float3 normal : NORMAL; //ノーマルマップ
    float2 uv : TEXCOORD;           //来たUV
    float4 shadowPos : TEXCOORD1;   //影の位置
    float3 worldPos : TEXCOORD2;    //ワールド座標
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

float2 Rotate(float2 v, float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return float2(c * v.x - s * v.y, s * v.x + c * v.y);
}

float rand(float2 co)
{
    return frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453);
}

float ShadowCalculation(float4 shadowPos, float bias)
{
    float3 posFromLightVP = shadowPos.xyz / shadowPos.w;
    float2 shadowUV = (posFromLightVP.xy + float2(1, -1)) * float2(0.5, -0.5);

    float isInside = step(0.0f, shadowUV.x) * step(shadowUV.x, 1.0f) *
                     step(0.0f, shadowUV.y) * step(shadowUV.y, 1.0f);

    float angle = rand(shadowUV * SHADOWSIZE) * 6.2831853f;
    float2 texelSize = float2(1.0f / SHADOWSIZE, 1.0f / SHADOWSIZE);
    float total = 0.0f;
    for (int i = 0; i < 8; ++i)
    {
        float2 rotated = Rotate(poissonDisk[i], angle);
        float2 shadowSampleUV = shadowUV + rotated * texelSize * 1.5f;
        float sampleDepth = shadowMap.SampleCmpLevelZero(shadowSampler, shadowSampleUV, posFromLightVP.z - bias);
        total += sampleDepth;
    }

    float shadowFactor = total / 8.0f;
    shadowFactor = lerp(1.0f, shadowFactor, isInside);
    return shadowFactor;
}

float4 pixel(VSOutput input) : SV_TARGET
{
    //テクスチャ
    float4 tex = _MainTex.Sample(texSampler, input.uv);

    //ライトの方向を逆に
    float3 lightVec = lightDir.xyz;
    float lightIntensity = saturate(dot(input.normal, -lightVec));

    float bias = ComputeShadowBias(input.normal, lightVec);

    //シャドウの計算
    float shadowFactor = ShadowCalculation(input.shadowPos, bias);
    
    float shadowPower = lightIntensity * shadowFactor;
    shadowPower = max(0.1f, shadowPower);

    //シャドウがかかっていれば光を減少させる (0.0f なら完全な影、1.0f なら影なし)
    float4 diffuse = diffuseColor * shadowPower;

    float4 lighting = lerp(ambientColor, diffuseColor, shadowPower);
    
    lighting *= tex;
    
    //カメラからの距離計算
    float distance = abs(length(input.worldPos - cameraEyePos.xyz));

    //0 ～ 1にマッピング
    float fogFactor = saturate((distance - fogStartEnd.x) / (fogStartEnd.y - fogStartEnd.x));
    fogFactor = min(0.9f, fogFactor);
    
    lighting = lerp(lighting, fogColor, fogFactor);

    lighting.a = cameraEyePos.w;

    return lighting;
}