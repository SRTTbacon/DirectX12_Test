cbuffer ModelConstantBuffer : register(b0)
{
    matrix modelMatrix;         //モデルマトリックス
    matrix viewMatrix;          //ビューマトリックス
    matrix projectionMatrix;    //プロジェクションマトリックス
    matrix lightViewProjMatrix; //ディレクショナルライトの情報
    float4 eyePos;              //カメラの位置
    float4 lightPos;            //ライトの位置
}

cbuffer MeshBuffer : register(b1)
{
    matrix meshMatrix;          //メッシュ単位のマトリックス
    matrix normalMatrix;        //モデルのスケール、回転などをinput.normalにも適応する用
    float time;             //時間情報（シェーダーで動的に更新）
}

struct VSInput
{
    float3 pos : POSITION;              //頂点座標
    float4 boneWeights : BONEWEIGHTS;   //各頂点のボーンの影響度
    uint4 boneIDs : BONEIDS;            //各頂点に影響を与えるボーンのインデックス (ボーンがないモデルはすべて0)
    uint vertexID : VERTEXID;           //頂点のID
    float3 normal : NORMAL;             //法線
    float2 uv : TEXCOORD;               //UV
    float3 tangent : TANGENT;           //接線
    float3 binormal : BINORMAL;         //従法線
};

struct VSOutput
{
    float4 svpos : SV_POSITION;     //スクリーン座標
    float3 normal : NORMAL;         //法線
    float2 uv : TEXCOORD;           //UV
    float4 shadowPos : TEXCOORD1;   //影の位置
    float3 worldPos : TEXCOORD2;    //ワールド座標
};

VSOutput vert(VSInput input)
{
    VSOutput output = (VSOutput)0;
    matrix temp = meshMatrix;
	float4 localPos = float4(input.pos, 1.0f);		    //頂点座標
    float4 meshPos = mul(temp, localPos);               //ローカル座標に変換
    float4 worldPos = mul(modelMatrix, meshPos);        //ワールド座標に変換
    float4 viewPos = mul(viewMatrix, worldPos);         //ビュー座標に変換
    float4 projPos = mul(projectionMatrix, viewPos);    //投影変換

    output.svpos = projPos;         //投影変換された座標
    output.worldPos = worldPos.xyz; //ワールド座標
    output.normal = normalize(mul(normalMatrix, float4(input.normal, 0.0f)).xyz);
    output.uv = input.uv;           //UV
    output.shadowPos = mul(lightViewProjMatrix, worldPos);
    return output; //ピクセルシェーダーに渡す
}
