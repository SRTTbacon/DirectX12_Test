#define BIAS 0.005f     //シャギー抑制のバイアス値

//定数バッファ
cbuffer LightBuffer : register(b0)
{
    float3 lightDir;        //ライトの方向
    float4 ambientColor;    //環境光の色
    float4 diffuseColor;    //拡散光の色
    float4 specularColor;   //スペキュラ光の色
};

struct VSOutput
{
	float4 svpos : SV_POSITION;         //頂点シェーダーから来た座標
    float3 normal : NORMAL;             //頂点シェーダーから北ノーマルマップ
	float4 color : COLOR;		        //頂点シェーダーから来た色
	float2 uv : TEXCOORD;		        //頂点シェーダーから来たUV
    float4 lightSpacePos : TEXCOORD1;   //ディレクショナルライトの情報
};

SamplerState texSampler : register(s0);	    //テクスチャ用サンプラー
Texture2D<float4> _MainTex : register(t0);	        //テクスチャ

SamplerComparisonState shadowSampler : register(s1);  //シャドウマップ用サンプラー
Texture2D shadowMap : register(t1);         //シャドウマップ

float4 pixel(VSOutput input) : SV_TARGET
{
    float4 tex = _MainTex.Sample(texSampler, input.uv);

    //法線を正規化
    float3 normal = normalize(input.normal);

    //ライトの方向を逆に
    float3 lightDirection = normalize(-lightDir);

    //拡散光の計算
    float NdotL = max(dot(normal, lightDirection), 0.0f);
    float3 diffuse = diffuseColor.rgb * NdotL;

    //環境光の計算
    float3 ambient = ambientColor.rgb;

    //シャドウマップのサンプリングと影判定 (軽量PCF 2x2)
    float shadowFactor = 0.0f;
    float2 texelSize = 1.0f / 2048.0f;

    for (int x = -1; x <= 1; x += 2)
    {
        for (int y = -1; y <= 1; y += 2)
        {
            shadowFactor += shadowMap.SampleCmpLevelZero(shadowSampler, input.lightSpacePos.xy + float2(x, y) * texelSize, input.lightSpacePos.z);
        }
    }
    //shadowFactor *= 0.25f;
    shadowFactor += 1.25f;

    //シャドウがかかっていれば光を減少させる (0.0f なら完全な影、1.0f なら影なし)
    float3 lighting = ambient + shadowFactor * diffuse;

    //ライティング結果とテクスチャカラーを掛け合わせる
    float4 finalColor = float4(lighting, 1.0f) * tex;

    return finalColor;
}
