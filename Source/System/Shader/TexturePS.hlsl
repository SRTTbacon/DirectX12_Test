#define BIAS 0.0006f        //シャギー抑制のバイアス値
#define SHADOWSIZE 8192.0f  //シャドウマップのサイズ

SamplerState texSampler : register(s0);                 //テクスチャ用サンプラー
SamplerComparisonState shadowSampler : register(s1);    //影用サンプラー
Texture2D<float4> _MainTex : register(t0);              //ベーステクスチャ
Texture2D<float4> _NormalMap : register(t1);            //ノーマルマップ
Texture2D<float> shadowMap : register(t2);              //シャドウマップ

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
    float4 svpos : SV_POSITION;         //座標
    float3 normal : NORMAL;             //ノーマル
    float2 uv : TEXCOORD0;              //UV
    float4 shadowPos : TEXCOORD1;       //影の位置
    float4 tanLightDir : TEXCOORD2;     //従法線
    float3 tanHalfWayVec : TEXCOORD3;   //接線
};

float ShadowCalculation(float4 shadowPos)
{
    float3 posFromLightVP = shadowPos.xyz / shadowPos.w;
    float2 shadowUV = (posFromLightVP.xy + float2(1.0f, -1.0f)) * float2(0.5f, -0.5f);

    //影の柔らかさを決定するための分割数(サンプリング数)
    int numSamples = 4; //サンプル数 (多いほど滑らかだが重い)
    float total = 0.0f;
    float2 texelSize = float2(1.0f / SHADOWSIZE, 1.0f / SHADOWSIZE);
    
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
    //テクスチャ
    float4 tex = _MainTex.Sample(texSampler, input.uv);

    //法線マップ
    float2 tiledUV = input.uv * float2(5.0f, 10.0f);
    float4 normalTexColor = _NormalMap.Sample(texSampler, tiledUV);
    
    //ライトの方向を逆に
    float3 lightVec = lightDir.xyz;
    float lightIntensity = saturate(dot(input.normal, -lightVec));
    //float lightIntensity = 0.8f;
    
    //シャドウの計算
    float shadowFactor = ShadowCalculation(input.shadowPos);
    //float shadowFactor = 1.0f;
    
    float4 shadowPower = shadowFactor * lightIntensity;

    //シャドウがかかっていれば光を減少させる (0.0f なら完全な影、1.0f なら影なし)
    float4 lighting = lerp(ambientColor, diffuseColor, shadowPower);

    //ライティング結果とテクスチャカラーを掛け合わせる
    float4 finalColor = lighting * tex;
    
    //float3 E = normalize(input.eyePos); // 視線ベクトル

    float3 N = 2.0f * normalTexColor.xyz - 1.0; // 法線マップからの法線
    N *= 0.15f;

    float3 blendedNormal = normalize(lerp(input.normal, N, saturate(0.5f)));

    //float3 R = reflect(-E, N); // 反射ベクトル
    //float amb = input.tanLightDir.w; // 環境光の強さ
    
    //finalColor *= max(0, dot(blendedNormal, -lightVec));
    //finalColor += 0.3f * pow(max(0, dot(R, L)), 8);
    
    finalColor.a = cameraEyePos.w;

    return finalColor;
}
