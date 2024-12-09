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
    // 1. 射影座標に変換
    float3 projCoords = shadowPos.xyz / shadowPos.w;
    projCoords.xy *= float2(1.0f, -1.0f);
    projCoords = projCoords * 0.5f + 0.5f;

    if (projCoords.x < 0.0f || projCoords.x > 1.0f ||
        projCoords.y < 0.0f || projCoords.y > 1.0f)
    {
        return 1.0f; // 範囲外は影なし
    }

    // 2. ブロッカー探索
    float texelSize = 1.0f / 8192.0f;
    float averageBlockerDepth = 0.0f;
    int blockerCount = 0;

    int searchRadius = 3; // ブロッカー探索範囲
    for (int x = -searchRadius; x <= searchRadius; x++)
    {
        for (int y = -searchRadius; y <= searchRadius; y++)
        {
            float2 offset = float2(x, y) * texelSize;
            float shadowDepth = shadowMap.SampleCmpLevelZero(shadowSampler, projCoords.xy + offset, projCoords.z - BIAS);
            if (shadowDepth < projCoords.z)
            {
                averageBlockerDepth += shadowDepth;
                blockerCount++;
            }
        }
    }

    if (blockerCount == 0)
    {
        return 1.0f; // 影なし
    }

    averageBlockerDepth /= blockerCount;

    // 3. 半影サイズの計算
    float penumbraSize = (projCoords.z - averageBlockerDepth) / averageBlockerDepth * 0.05f;

    // 4. PCFによるシャドウサンプリング
    int filterRadius = saturate(penumbraSize / texelSize);
    float shadowFactor = 0.0f;
    int samples = 0;

    for (int x1 = -filterRadius; x1 <= filterRadius; x1++)
    {
        for (int y = -filterRadius; y <= filterRadius; y++)
        {
            float2 offset = float2(x1, y) * texelSize;
            shadowFactor += shadowMap.SampleCmpLevelZero(shadowSampler, projCoords.xy + offset, projCoords.z - BIAS);
            samples++;
        }
    }

    return shadowFactor / samples;
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