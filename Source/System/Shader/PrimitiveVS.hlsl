cbuffer ModelConstantBuffer : register(b0)
{
    matrix modelMatrix;         //モデルマトリックス
    matrix viewMatrix;          //ビューマトリックス
    matrix projectionMatrix;    //プロジェクションマトリックス
    matrix lightViewProjMatrix; //ディレクショナルライトの情報
    matrix normalMatrix; //モデルのスケール、回転などをinput.normalにも適応する用
}

struct VSInput
{
    float3 pos : POSITION;  //頂点座標
    float3 normal : NORMAL; // 法線
    float2 uv : TEXCOORD; // UV
    float4 color : COLOR;   //頂点色
};

struct VSOutput
{
    float4 svpos : SV_POSITION; //座標
    float3 normal : NORMAL; //ノーマルマップ
    float4 color : COLOR; //色
    float2 uv : TEXCOORD; //UV
    float4 lightSpacePos : TEXCOORD1; //ディレクショナルライト
};

VSOutput vert(VSInput input)
{
    VSOutput output = (VSOutput)0;
	
	float4 localPos = float4(input.pos, 1.0f);		    //頂点座標
    float4 worldPos = mul(modelMatrix, localPos);       //ワールド座標に変換
    float4 viewPos = mul(viewMatrix, worldPos);         //ビュー座標に変換
    float4 projPos = mul(projectionMatrix, viewPos);    //投影変換

    output.svpos = projPos; //投影変換された座標
    output.normal = mul(float4(input.normal, 1.0f), normalMatrix).xyz; //ノーマルマップ
    output.color = input.color; //頂点色
    output.uv = input.uv; //UV
    float4 lightSpacePos = mul(lightViewProjMatrix, worldPos);
    output.lightSpacePos = lightSpacePos / lightSpacePos.w; //ライト空間への変換
    return output; //ピクセルシェーダーに渡す
}
