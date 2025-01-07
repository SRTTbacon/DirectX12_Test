#define BIAS 0.0006f        //シャギー抑制のバイアス値
#define SHADOWSIZE 8192.0f  //シャドウマップのサイズ

SamplerState texSampler : register(s0);                 //テクスチャ用サンプラー
SamplerComparisonState shadowSampler : register(s1);    //テクスチャ用サンプラー
Texture2D<float4> _MainTex : register(t0);              //テクスチャ
Texture2D<float4> _NormalMap : register(t1);            //ノーマルマップ
Texture2D<float> shadowMap : register(t2);              //シャドウマップ

//定数バッファ
cbuffer LightBuffer : register(b0)
{
    float4 lightDir;        //ライトの方向
    float4 ambientColor;    //影の色
    float4 diffuseColor;    //標準の色
};

struct VSOutput
{
    float4 svpos : SV_POSITION;     //座標
    float3 normal : NORMAL;         //ノーマル
    float2 uv : TEXCOORD0;           //UV
    float4 shadowPos : TEXCOORD1;   //影の位置
    float3 tanLightDir : TEXCOORD2; //従法線
    float3 tanHalfWayVec : TEXCOORD3; //接線
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
    float3 normal = normalTexColor.xyz * 2 - 1.0f;

    normal = normalize(normal);
    
    //normal *= 0.1f;

    //ライトの方向を逆に
    float lightIntensity = saturate(dot(input.normal, -lightDir.xyz));
    //float lightIntensity = saturate(dot(normal, input.tanLightDir));
    
    //シャドウの計算
    float shadowFactor = ShadowCalculation(input.shadowPos);
    
    float4 diffuse = diffuseColor * lightIntensity;

    //シャドウがかかっていれば光を減少させる (0.0f なら完全な影、1.0f なら影なし)
    float4 lighting = ambientColor + shadowFactor * diffuse;

    //ライティング結果とテクスチャカラーを掛け合わせる
    float4 finalColor = lighting * tex;
    
    //スペキュラ
    float specularIntensity = pow(saturate(dot(normal, -input.tanHalfWayVec)), 1.0f);
    float4 specular = float4(1.0f, 1.0f, 1.0f, 1.0f) * specularIntensity * 0.25f;
    specular.r = saturate(specular.r);
    specular.g = saturate(specular.g);
    specular.b = saturate(specular.b);
    specular.a = saturate(specular.a);
    //float4 specular = float4(1.0f, 1.0f, 1.0f, 1.0f) * pow(saturate(dot(input.tanHalfWayVec, normal)), 0.5f) * 0.15f;
    //finalColor -= specular;

    return finalColor;
}
